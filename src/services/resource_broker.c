#include "nova_runtime.h"
#include "nova_app_security_protocol.h"
#include "nova_resource_broker_protocol.h"

struct broker_session {
    int fd;
    unsigned int peer_pid;
    unsigned int client_id;
    unsigned long long app_generation;
    unsigned long long policy_generation;
    unsigned long long effective_capabilities;
    unsigned long long last_activity_sec;
    char app_id[NOVA_APP_ID_BYTES];
};

/* Image-owned catalog. Applications never provide filenames. */
struct broker_resource {
    unsigned int id;
    const char *path;
};
static const struct broker_resource g_catalog[] = {
    {NOVA_RESOURCE_ID_DEMO,NOVA_RESOURCE_BROKER_DEMO_SOURCE},
    {NOVA_RESOURCE_ID_NOTES,NOVA_RESOURCE_BROKER_NOTES_SOURCE},
};
static const char *catalog_path_for_id(unsigned int id) {
    for(unsigned int i=0U;
        i<sizeof(g_catalog)/sizeof(g_catalog[0]);i++)
        if(id==g_catalog[i].id) return g_catalog[i].path;
    return 0;
}
static int g_authority_fd=-1;
static unsigned int g_authority_request_id=1U;
static unsigned long g_connections;
static unsigned long g_disconnects;
static unsigned long g_idle_expirations;
static unsigned long g_clock_failures;
static unsigned long g_auth_denials;
static unsigned long g_requests;
static unsigned long g_replies;
static unsigned long g_unsupported;
static unsigned long g_grants;
static unsigned long g_grant_denials;
/* Acceptance-only instrumentation. No test object/path exists in normal builds. */
#ifdef NOVA_PHASE18_STEP3_ADVERSARIAL
#define NOVA_STEP3_TEST_DIRECTORY "/run/nova/step3-test-directory"
#define NOVA_STEP3_TEST_FIFO "/run/nova/step3-test-fifo"
#define NOVA_STEP3_TEST_OVERSIZE "/run/nova/step3-test-oversize"
static unsigned long g_step3_type_denials;
static unsigned long g_step3_size_denials;
static int step3_create_fixture_objects(void) {
    if(nova_mkdir(NOVA_STEP3_TEST_DIRECTORY,0700)<0) return 0;
    if(nova_mknod(NOVA_STEP3_TEST_FIFO,0010000UL|0600UL,0,0)<0) return 0;
    long fd=nova_open_create_excl(NOVA_STEP3_TEST_OVERSIZE,0600);
    if(fd<0) return 0;
    static const unsigned char zeros[1024]={0};
    int ok=1;
    for(unsigned int i=0;i<64U;i++) {
        if(nova_fd_write_all((int)fd,zeros,sizeof(zeros))!=0) {ok=0;break;}
    }
    if(ok && nova_fd_write_all((int)fd,"x",1)!=0) ok=0;
    (void)nova_close((int)fd);
    return ok;
}
static const char *step3_fixture_path(unsigned int request_id) {
    if(request_id==2U) return NOVA_STEP3_TEST_DIRECTORY;
    if(request_id==3U) return NOVA_STEP3_TEST_FIFO;
    if(request_id==4U) return NOVA_STEP3_TEST_OVERSIZE;
    return NOVA_RESOURCE_BROKER_DEMO_SOURCE;
}
#endif


static void copy_string(char *dst,unsigned long cap,const char *src) {
    if(!dst || !cap) return;
    unsigned long i=0U;
    while(src && src[i] && i+1UL<cap){ dst[i]=src[i]; i++; }
    dst[i]='\0';
    for(i++;i<cap;i++) dst[i]='\0';
}

static void fill_unix_addr(struct nova_sockaddr_un *addr,const char *path) {
    nova_zero(addr,sizeof(*addr));
    addr->sun_family=NOVA_AF_UNIX;
    unsigned long i=0U;
    while(path && path[i] && i+1UL<sizeof(addr->sun_path)){ addr->sun_path[i]=path[i]; i++; }
    addr->sun_path[i]='\0';
}

static int app_id_valid(const char app_id[NOVA_APP_ID_BYTES]) {
    if(!app_id) return 0;
    unsigned long nul=NOVA_APP_ID_BYTES;
    for(unsigned long i=0U;i<NOVA_APP_ID_BYTES;i++) if(app_id[i]=='\0'){ nul=i; break; }
    if(nul==0UL || nul==NOVA_APP_ID_BYTES) return 0;
    for(unsigned long i=0U;i<nul;i++){
        unsigned char c=(unsigned char)app_id[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')) return 0;
    }
    return 1;
}

static int peer_pid_for_fd(int fd,unsigned int *pid_out) {
    if(fd<0 || !pid_out) return 0;
    struct nova_ucred cred;
    unsigned int len=(unsigned int)sizeof(cred);
    nova_zero(&cred,sizeof(cred));
    long rc=nova_getsockopt(fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED,&cred,&len);
    if(rc<0 || len!=(unsigned int)sizeof(cred) || cred.pid<=0) return 0;
    *pid_out=(unsigned int)cred.pid;
    return 1;
}

static int authority_reply_header_valid(const struct nova_app_security_reply *reply,unsigned int request_id) {
    return reply && reply->magic==NOVA_APP_SECURITY_PROTOCOL_MAGIC &&
           reply->version==NOVA_APP_SECURITY_PROTOCOL_VERSION &&
           reply->size==(unsigned int)sizeof(*reply) && reply->request_id==request_id;
}

static int authority_send_request(int fd,unsigned int opcode,unsigned int process_id,
                                  struct nova_app_security_reply *reply) {
    if(fd<0 || !reply) return 0;
    struct nova_app_security_request request;
    nova_zero(&request,sizeof(request));
    request.magic=NOVA_APP_SECURITY_PROTOCOL_MAGIC;
    request.version=NOVA_APP_SECURITY_PROTOCOL_VERSION;
    request.opcode=opcode;
    request.size=(unsigned int)sizeof(request);
    request.request_id=g_authority_request_id++;
    if(!g_authority_request_id) g_authority_request_id=1U;
    request.process_id=process_id;
    if(nova_fd_write_all(fd,&request,sizeof(request))!=0) return 0;
    nova_zero(reply,sizeof(*reply));
    long n=nova_read(fd,reply,sizeof(*reply));
    return n==(long)sizeof(*reply) && authority_reply_header_valid(reply,request.request_id);
}

static void disconnect_authority(void) {
    if(g_authority_fd>=0) (void)nova_close(g_authority_fd);
    g_authority_fd=-1;
}

static int connect_authority(void) {
    if(g_authority_fd>=0) return 1;
    struct nova_sockaddr_un addr;
    fill_unix_addr(&addr,NOVA_RESOURCE_BROKER_AUTHORITY_SOCKET_PATH);
    long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0);
    if(fd<0) return 0;
    if(nova_connect((int)fd,&addr,sizeof(addr))<0){ (void)nova_close((int)fd); return 0; }
    struct nova_app_security_reply reply;
    if(!authority_send_request((int)fd,NOVA_APP_SECURITY_OP_HELLO,0U,&reply) ||
       reply.opcode!=NOVA_APP_SECURITY_OP_HELLO_REPLY || reply.result!=NOVA_APP_SECURITY_RESULT_OK ||
       reply.process_id!=0U || reply.client_id!=0U || reply.security_flags!=0U ||
       reply.app_generation!=0ULL || reply.declared_capabilities!=0ULL ||
       reply.effective_capabilities!=0ULL || reply.app_id[0]!='\0'){
        (void)nova_close((int)fd); return 0;
    }
    g_authority_fd=(int)fd;
    return 1;
}

static int lookup_live_principal(unsigned int peer_pid,struct nova_app_security_reply *reply) {
    if(!peer_pid || !reply) return 0;
    for(unsigned int attempt=0U;attempt<2U;attempt++){
        if(!connect_authority()) continue;
        if(!authority_send_request(g_authority_fd,NOVA_APP_SECURITY_OP_LOOKUP_PID,peer_pid,reply)){
            disconnect_authority();
            continue;
        }
        if(reply->opcode!=NOVA_APP_SECURITY_OP_PRINCIPAL || reply->result!=NOVA_APP_SECURITY_RESULT_OK ||
           reply->process_id!=peer_pid || reply->client_id==0U || reply->app_generation==0ULL ||
           reply->policy_generation==0ULL ||
           reply->security_flags!=(NOVA_APP_SECURITY_FLAG_AUTHENTICATED|NOVA_APP_SECURITY_FLAG_POLICY_BOUND) ||
           (reply->effective_capabilities&~reply->declared_capabilities)!=0ULL ||
           !app_id_valid(reply->app_id)) return 0;
        return 1;
    }
    return 0;
}

/* Idle policy uses monotonic kernel time, never wall clock. */
#define NOVA_RESOURCE_BROKER_IDLE_SECONDS 120ULL
static long broker_now_seconds(void) {
    struct nova_timespec now={0,0};
    if(nova_clock_gettime(NOVA_CLOCK_MONOTONIC,&now)<0 || now.tv_sec<0) return -1;
    return now.tv_sec;
}

static unsigned int connected_count(const struct broker_session sessions[NOVA_RESOURCE_BROKER_MAX_CLIENTS]) {
    unsigned int count=0U;
    for(unsigned int i=0U;i<NOVA_RESOURCE_BROKER_MAX_CLIENTS;i++) if(sessions[i].fd>=0) count++;
    return count;
}

static void write_state(const struct broker_session sessions[NOVA_RESOURCE_BROKER_MAX_CLIENTS],const char *stage) {
    long fd=nova_open_write(NOVA_RESOURCE_BROKER_STATE_PATH,0644);
    if(fd<0) return;
    nova_fd_putln((int)fd,"schema=1");
    nova_fd_puts((int)fd,"stage="); nova_fd_putln((int)fd,stage?stage:"UNKNOWN");
    nova_fd_putln((int)fd,"protocol=1");
    nova_fd_putln((int)fd,"transport=AF_UNIX_SEQPACKET");
    nova_fd_putln((int)fd,"authentication=SO_PEERCRED_NOVAD_LIVE_PRINCIPAL");
    nova_fd_putln((int)fd,"hello_grants=none");
    nova_fd_putln((int)fd,"grant_surface=fixed-demo-readonly-fd");
    nova_fd_puts((int)fd,"authority_connected="); nova_fd_putln((int)fd,g_authority_fd>=0?"yes":"no");
    nova_fd_puts((int)fd,"connected_sessions="); nova_fd_put_u64((int)fd,connected_count(sessions)); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"connections="); nova_fd_put_u64((int)fd,g_connections); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"disconnects="); nova_fd_put_u64((int)fd,g_disconnects); nova_fd_putln((int)fd,"");
    nova_fd_putln((int)fd,"idle_timeout_seconds=120");
    nova_fd_puts((int)fd,"idle_expirations="); nova_fd_put_u64((int)fd,g_idle_expirations); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"clock_failures="); nova_fd_put_u64((int)fd,g_clock_failures); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"auth_denials="); nova_fd_put_u64((int)fd,g_auth_denials); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"requests="); nova_fd_put_u64((int)fd,g_requests); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"replies="); nova_fd_put_u64((int)fd,g_replies); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"unsupported="); nova_fd_put_u64((int)fd,g_unsupported); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"grants="); nova_fd_put_u64((int)fd,g_grants); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"grant_denials="); nova_fd_put_u64((int)fd,g_grant_denials); nova_fd_putln((int)fd,"");
#ifdef NOVA_PHASE18_STEP3_ADVERSARIAL
    nova_fd_putln((int)fd,"step3_fixture_mode=adversarial-only");
    nova_fd_puts((int)fd,"step3_type_denials="); nova_fd_put_u64((int)fd,g_step3_type_denials); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"step3_size_denials="); nova_fd_put_u64((int)fd,g_step3_size_denials); nova_fd_putln((int)fd,"");
#endif
    (void)nova_fsync((int)fd);
    (void)nova_close((int)fd);
}

static void fill_broker_reply(struct nova_resource_broker_reply *reply,
                              const struct nova_resource_broker_request *request,
                              unsigned int opcode,unsigned int result,
                              const struct broker_session *session) {
    nova_zero(reply,sizeof(*reply));
    reply->magic=NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC;
    reply->version=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION;
    reply->opcode=opcode;
    reply->size=(unsigned int)sizeof(*reply);
    reply->request_id=request?request->request_id:0U;
    reply->result=result;
    if(session){
        reply->broker_flags=NOVA_RESOURCE_BROKER_FLAG_AUTHENTICATED|
                            NOVA_RESOURCE_BROKER_FLAG_POLICY_BOUND|
                            NOVA_RESOURCE_BROKER_FLAG_NO_GRANT_SURFACE;
        reply->process_id=session->peer_pid;
        reply->client_id=session->client_id;
        reply->app_generation=session->app_generation;
        reply->policy_generation=session->policy_generation;
        reply->effective_capabilities=session->effective_capabilities;
        copy_string(reply->app_id,sizeof(reply->app_id),session->app_id);
    }
}

static int request_valid(const struct nova_resource_broker_request *request) {
    if(!request) return 0;
    if(request->magic!=NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC ||
       request->version!=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION ||
       request->size!=(unsigned int)sizeof(*request) || request->request_id==0U) return 0;
    if(request->reserved1 || request->reserved2) return 0;
    if(request->opcode==NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO)
        return request->reserved0!=0U;
    if(request->reserved0) return 0;
    return 1;
}

static int send_reply(int fd,const struct nova_resource_broker_reply *reply) {
    long n=nova_write(fd,reply,sizeof(*reply));
    if(n==(long)sizeof(*reply)){ g_replies++; return 0; }
    return -1;
}

/* A successful grant is exactly one read-only FD on the same seqpacket as
 * the complete reply. Kernel duplicates the open file description using
 * SCM_RIGHTS. All failure replies carry NO ancillary data. */
static int send_readonly_fd(int socket_fd,const struct nova_resource_broker_reply *reply,int resource_fd) {
    struct nova_iovec iov;
    struct nova_msghdr msg;
    union {
        unsigned long align;
        unsigned char bytes[NOVA_CMSG_SPACE(sizeof(int))];
    } control;
    nova_zero(&iov,sizeof(iov)); nova_zero(&msg,sizeof(msg)); nova_zero(&control,sizeof(control));
    iov.iov_base=(void *)reply; iov.iov_len=sizeof(*reply);
    msg.msg_iov=&iov; msg.msg_iovlen=1;
    msg.msg_control=control.bytes; msg.msg_controllen=sizeof(control.bytes);
    struct nova_cmsghdr *cmsg=(struct nova_cmsghdr *)control.bytes;
    cmsg->cmsg_level=NOVA_SOL_SOCKET;
    cmsg->cmsg_type=NOVA_SCM_RIGHTS;
    cmsg->cmsg_len=NOVA_CMSG_LEN(sizeof(int));
    nova_copy(control.bytes+NOVA_CMSG_DATA_OFFSET,&resource_fd,sizeof(resource_fd));
    long n=nova_sendmsg(socket_fd,&msg,NOVA_MSG_NOSIGNAL);
    if(n==(long)sizeof(*reply)){ g_replies++; g_grants++; return 0; }
    return -1;
}

static int live_session_matches(const struct broker_session *session,
                                const struct nova_app_security_reply *principal) {
    if(!session || !principal) return 0;
    if(session->peer_pid!=principal->process_id ||
       session->client_id!=principal->client_id ||
       session->app_generation!=principal->app_generation ||
       session->policy_generation!=principal->policy_generation ||
       !nova_streq(session->app_id,principal->app_id)) return 0;
    /* Both current and originally authenticated session must authorize it;
     * reconnect required following a policy/principal change. */
    return (session->effective_capabilities&NOVA_APP_CAP_RESOURCE_DEMO_READ)!=0ULL &&
           (principal->effective_capabilities&NOVA_APP_CAP_RESOURCE_DEMO_READ)!=0ULL;
}


static int live_session_matches_notes(const struct broker_session *session,
                                const struct nova_app_security_reply *principal) {
    if(!session || !principal) return 0;
    if(session->peer_pid!=principal->process_id ||
       session->client_id!=principal->client_id ||
       session->app_generation!=principal->app_generation ||
       session->policy_generation!=principal->policy_generation ||
       !nova_streq(session->app_id,principal->app_id)) return 0;
    /* Both current and originally authenticated session must authorize it;
     * reconnect required following a policy/principal change. */
    return (session->effective_capabilities&NOVA_APP_CAP_RESOURCE_NOTES_READ)!=0ULL &&
           (principal->effective_capabilities&NOVA_APP_CAP_RESOURCE_NOTES_READ)!=0ULL;
}

/* Step-3 R1: type and size gate against the already-opened FD.
 * statx(AT_EMPTY_PATH) addresses exactly this open file description,
 * avoiding a separate pathname check / TOCTOU window. Fail closed if
 * the kernel cannot provide authoritative mode+size attributes. */
#define NOVA_RESOURCE_DEMO_MAX_BYTES 65536ULL
static int grant_fd_regular_bounded(int fd) {
    if(fd<0) return 0;
    struct nova_statx st;
    nova_zero(&st,sizeof(st));
    long rc=nova_syscall5(NOVA_NR_statx,fd,(long)"",
                          NOVA_AT_EMPTY_PATH,NOVA_STATX_TYPE|NOVA_STATX_SIZE,(long)&st);
    if(rc!=0) return 0;
    if((st.stx_mask&(NOVA_STATX_TYPE|NOVA_STATX_SIZE))!=
       (NOVA_STATX_TYPE|NOVA_STATX_SIZE)) return 0;
    if((st.stx_mode&NOVA_S_IFMT)!=NOVA_S_IFREG) {
#ifdef NOVA_PHASE18_STEP3_ADVERSARIAL
        g_step3_type_denials++;
#endif
        return 0;
    }
    if(st.stx_size>NOVA_RESOURCE_DEMO_MAX_BYTES) {
#ifdef NOVA_PHASE18_STEP3_ADVERSARIAL
        g_step3_size_denials++;
#endif
        return 0;
    }
    return 1;
}

static int grant_fixed_readonly_resource(struct broker_session *session,
                                         const struct nova_resource_broker_request *request) {
    struct nova_resource_broker_reply reply;
    struct nova_app_security_reply live;
    /* Re-query novad immediately before opening; an accepted socket alone
     * is NOT durable authority after app restart or policy changes. */
    int notes_selected=request->opcode==NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO &&
                       request->reserved0==NOVA_RESOURCE_ID_NOTES;
    if(!lookup_live_principal(session->peer_pid,&live) ||
       !(notes_selected ? live_session_matches_notes(session,&live) :
                           live_session_matches(session,&live))) {
        g_grant_denials++;
        fill_broker_reply(&reply,request,NOVA_RESOURCE_BROKER_OP_ERROR,NOVA_RESOURCE_BROKER_RESULT_DENIED,session);
        return send_reply(session->fd,&reply);
    }
    /* Static image-owned path; no user-selected path or directory traversal.
     * O_NOFOLLOW refuses a malicious final symlink. O_NONBLOCK avoids FIFO
     * blocking should image contents be unexpectedly replaced. */
    const char *source=0;
    if(request->opcode==NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO) {
        source=catalog_path_for_id(request->reserved0);
        if(!source) {
            g_grant_denials++;
            fill_broker_reply(&reply,request,NOVA_RESOURCE_BROKER_OP_ERROR,
                              NOVA_RESOURCE_BROKER_RESULT_DENIED,session);
            return send_reply(session->fd,&reply);
        }
    } else {
#ifdef NOVA_PHASE18_STEP3_ADVERSARIAL
        source=step3_fixture_path(request->request_id);
#else
        source=NOVA_RESOURCE_BROKER_DEMO_SOURCE;
#endif
    }
    long resource_fd=nova_syscall4(NOVA_NR_openat,NOVA_AT_FDCWD,
        (long)source,
        NOVA_O_RDONLY|NOVA_O_CLOEXEC|NOVA_O_NOFOLLOW|NOVA_O_NONBLOCK,0L);
    if(resource_fd<0){
        g_grant_denials++;
        fill_broker_reply(&reply,request,NOVA_RESOURCE_BROKER_OP_ERROR,NOVA_RESOURCE_BROKER_RESULT_INTERNAL,session);
        return send_reply(session->fd,&reply);
    }
    /* Never pass a directory, FIFO, socket, device or oversized file. */
    if(!grant_fd_regular_bounded((int)resource_fd)){
        (void)nova_close((int)resource_fd);
        g_grant_denials++;
        fill_broker_reply(&reply,request,NOVA_RESOURCE_BROKER_OP_ERROR,NOVA_RESOURCE_BROKER_RESULT_INTERNAL,session);
        return send_reply(session->fd,&reply);
    }
    fill_broker_reply(&reply,request,NOVA_RESOURCE_BROKER_OP_GRANT_FD,NOVA_RESOURCE_BROKER_RESULT_OK,session);
    reply.broker_flags &= ~NOVA_RESOURCE_BROKER_FLAG_NO_GRANT_SURFACE;
    reply.broker_flags |= NOVA_RESOURCE_BROKER_FLAG_FD_READONLY;
    int rc=send_readonly_fd(session->fd,&reply,(int)resource_fd);
    (void)nova_close((int)resource_fd);
    return rc;
}

static int process_session_packet(struct broker_session *session) {
    if(!session || session->fd<0) return -1;
    struct nova_resource_broker_request request;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    nova_zero(&request,sizeof(request)); nova_zero(&iov,sizeof(iov)); nova_zero(&msg,sizeof(msg));
    iov.iov_base=&request; iov.iov_len=sizeof(request); msg.msg_iov=&iov; msg.msg_iovlen=1;
    long n=nova_recvmsg(session->fd,&msg,0);
    if(n==-NOVA_EAGAIN || n==-NOVA_EINTR) return 0;
    if(n<=0) return -1;
    g_requests++;

    struct nova_resource_broker_reply reply;
    if(n!=(long)sizeof(request) || (msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)) || !request_valid(&request)){
        fill_broker_reply(&reply,&request,NOVA_RESOURCE_BROKER_OP_ERROR,NOVA_RESOURCE_BROKER_RESULT_BAD_REQUEST,session);
        return send_reply(session->fd,&reply);
    }
    long activity_sec=broker_now_seconds();
    if(activity_sec<0){g_clock_failures++;return -1;}
    session->last_activity_sec=(unsigned long long)activity_sec;
    if(request.opcode==NOVA_RESOURCE_BROKER_OP_OPEN_DEMO_RO)
        return grant_fixed_readonly_resource(session,&request);
    if(request.opcode==NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO)
        return grant_fixed_readonly_resource(session,&request);
    if(request.opcode!=NOVA_RESOURCE_BROKER_OP_HELLO){
        g_unsupported++;
        fill_broker_reply(&reply,&request,NOVA_RESOURCE_BROKER_OP_ERROR,NOVA_RESOURCE_BROKER_RESULT_UNSUPPORTED,session);
        return send_reply(session->fd,&reply);
    }
    fill_broker_reply(&reply,&request,NOVA_RESOURCE_BROKER_OP_SESSION,NOVA_RESOURCE_BROKER_RESULT_OK,session);
    return send_reply(session->fd,&reply);
}

static void disconnect_session(struct broker_session sessions[NOVA_RESOURCE_BROKER_MAX_CLIENTS],unsigned int slot) {
    if(slot>=NOVA_RESOURCE_BROKER_MAX_CLIENTS || sessions[slot].fd<0) return;
    (void)nova_close(sessions[slot].fd);
    nova_zero(&sessions[slot],sizeof(sessions[slot]));
    sessions[slot].fd=-1;
    g_disconnects++;
}

/* Expire only after the existing sockets' poll events have been processed.
 * A monotonic-clock failure closes sessions rather than leaving slots held. */
static void expire_idle_sessions(struct broker_session sessions[NOVA_RESOURCE_BROKER_MAX_CLIENTS]) {
    long now=broker_now_seconds();
    if(now<0){
        g_clock_failures++;
        for(unsigned int i=0U;i<NOVA_RESOURCE_BROKER_MAX_CLIENTS;i++)
            disconnect_session(sessions,i);
        return;
    }
    for(unsigned int i=0U;i<NOVA_RESOURCE_BROKER_MAX_CLIENTS;i++){
        if(sessions[i].fd<0) continue;
        unsigned long long last=sessions[i].last_activity_sec;
        if((unsigned long long)now>=last &&
           (unsigned long long)now-last>=NOVA_RESOURCE_BROKER_IDLE_SECONDS){
            disconnect_session(sessions,i);
            g_idle_expirations++;
        }
    }
}

static void accept_sessions(int listener,struct broker_session sessions[NOVA_RESOURCE_BROKER_MAX_CLIENTS]) {
    /* A bounded batch prevents an unbounded accept stream from starving
     * already-connected clients in the same poll cycle. */
    for(unsigned int accepted=0U;accepted<NOVA_RESOURCE_BROKER_MAX_CLIENTS;accepted++){
        long fd=nova_accept4(listener,NOVA_SOCK_NONBLOCK);
        if(fd==-NOVA_EAGAIN || fd==-NOVA_EINTR) return;
        if(fd<0) return;

        /* No principal/authority RPC is necessary if capacity is exhausted.
         * This accepted fd is owned by the broker and MUST be closed. */
        if(connected_count(sessions)>=NOVA_RESOURCE_BROKER_MAX_CLIENTS){
            (void)nova_close((int)fd);
            continue;
        }

        unsigned int peer_pid=0U;
        struct nova_app_security_reply principal;
        if(!peer_pid_for_fd((int)fd,&peer_pid) || !lookup_live_principal(peer_pid,&principal)){
            g_auth_denials++;
            (void)nova_close((int)fd);
            continue;
        }

        long connected_sec=broker_now_seconds();
        if(connected_sec<0){
            g_clock_failures++;
            (void)nova_close((int)fd);
            continue;
        }
        int placed=0;
        for(unsigned int i=0U;i<NOVA_RESOURCE_BROKER_MAX_CLIENTS;i++){
            if(sessions[i].fd>=0) continue;
            sessions[i].fd=(int)fd;
            sessions[i].peer_pid=peer_pid;
            sessions[i].client_id=principal.client_id;
            sessions[i].app_generation=principal.app_generation;
            sessions[i].policy_generation=principal.policy_generation;
            sessions[i].effective_capabilities=principal.effective_capabilities;
            sessions[i].last_activity_sec=(unsigned long long)connected_sec;
            copy_string(sessions[i].app_id,sizeof(sessions[i].app_id),principal.app_id);
            g_connections++;
            placed=1;
            break;
        }
        if(!placed) (void)nova_close((int)fd);
    }
}

static int setup_listener(void) {
    struct nova_sockaddr_un addr;
    fill_unix_addr(&addr,NOVA_RESOURCE_BROKER_HOST_SOCKET_PATH);
    (void)nova_mkdir("/run/nova/app",0755);
    if(nova_fchmodat("/run/nova/app",0755)<0) return -1;
    (void)nova_unlink(NOVA_RESOURCE_BROKER_HOST_SOCKET_PATH);
    long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,0);
    if(fd<0) return -1;
    if(nova_bind((int)fd,&addr,sizeof(addr))<0 ||
       nova_fchmodat(NOVA_RESOURCE_BROKER_HOST_SOCKET_PATH,0666)<0 ||
       nova_listen((int)fd,(int)NOVA_RESOURCE_BROKER_MAX_CLIENTS)<0){
        (void)nova_close((int)fd);
        (void)nova_unlink(NOVA_RESOURCE_BROKER_HOST_SOCKET_PATH);
        return -1;
    }
    return (int)fd;
}

int nova_main(void) {
    struct broker_session sessions[NOVA_RESOURCE_BROKER_MAX_CLIENTS];
    nova_zero(sessions,sizeof(sessions));
    for(unsigned int i=0U;i<NOVA_RESOURCE_BROKER_MAX_CLIENTS;i++) sessions[i].fd=-1;

#ifdef NOVA_PHASE18_STEP3_ADVERSARIAL
    if(!step3_create_fixture_objects()) {
        write_state(sessions,"FAILED");
        nova_putln("[nova-resource-broker] step3-fixtures=FAILED");
        return 1;
    }
    nova_putln("[nova-resource-broker] step3-fixtures=READY");
#endif
    int listener=setup_listener();
    if(listener<0){
        write_state(sessions,"FAILED");
        nova_putln("[nova-resource-broker] stage=FAILED reason=app_listener");
        return 1;
    }
    write_state(sessions,"LISTENING");
    nova_putln("[nova-resource-broker] stage=LISTENING protocol=1 grants=fixed-demo-ro auth=SO_PEERCRED+novad-live-principal");

    for(;;){
        struct nova_pollfd fds[1U+NOVA_RESOURCE_BROKER_MAX_CLIENTS];
        nova_zero(fds,sizeof(fds));
        fds[0].fd=listener; fds[0].events=NOVA_POLLIN;
        for(unsigned int i=0U;i<NOVA_RESOURCE_BROKER_MAX_CLIENTS;i++){
            fds[1U+i].fd=sessions[i].fd;
            fds[1U+i].events=sessions[i].fd>=0?(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP):0;
        }
        struct nova_timespec timeout={1,0};
        long rc=nova_poll(fds,1U+NOVA_RESOURCE_BROKER_MAX_CLIENTS,&timeout);
        if(rc<0 && rc!=-NOVA_EINTR) continue;
        if(fds[0].revents&NOVA_POLLIN) accept_sessions(listener,sessions);
        for(unsigned int i=0U;i<NOVA_RESOURCE_BROKER_MAX_CLIENTS;i++){
            if(sessions[i].fd<0) continue;
            short rev=fds[1U+i].revents;
            if(rev&(NOVA_POLLERR|NOVA_POLLHUP)) disconnect_session(sessions,i);
            else if((rev&NOVA_POLLIN) && process_session_packet(&sessions[i])<0) disconnect_session(sessions,i);
        }
        expire_idle_sessions(sessions);
        write_state(sessions,"LISTENING");
    }
}
