/* NovaOS Phase-18 Step-7 R2: real-guest 120s idle expiry and renewal.
 * A single authenticated sandboxed process owns four broker sessions:
 * two deliberately idle, two kept active with valid HELLO every 20s.
 * One idle connection sends malformed framing halfway through, which
 * MUST NOT renew the 120s inactivity deadline.
 */
#include "nova_runtime.h"
#include "nova_resource_broker_protocol.h"

#define SESSIONS 4U
#define HEARTBEATS 6U
#define MAX_READ_ATTEMPTS 25U

static void broker_address(struct nova_sockaddr_un *a) {
    const char *p=NOVA_RESOURCE_BROKER_APP_SOCKET_PATH;
    nova_zero(a,sizeof(*a)); a->sun_family=NOVA_AF_UNIX;
    for(unsigned long i=0;p[i] && i+1U<sizeof(a->sun_path);i++) a->sun_path[i]=p[i];
}

static int connect_broker(void) {
    struct nova_sockaddr_un addr;
    broker_address(&addr);
    long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0);
    if(fd<0) return -1;
    if(nova_connect((int)fd,&addr,sizeof(addr))<0){(void)nova_close((int)fd);return -1;}
    return (int)fd;
}

static void make_request(struct nova_resource_broker_request *r,unsigned int id,int malformed) {
    nova_zero(r,sizeof(*r));
    r->magic=malformed ? 0U : NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC;
    r->version=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION;
    r->size=(unsigned int)sizeof(*r);
    r->opcode=NOVA_RESOURCE_BROKER_OP_HELLO;
    r->request_id=id;
}

/* One full reply, never any SCM_RIGHTS ancillary data. */
static int request_reply(int fd,unsigned int id,int malformed) {
    struct nova_resource_broker_request r;
    make_request(&r,id,malformed);
    if(nova_write(fd,&r,sizeof(r))!=(long)sizeof(r)) return 0;
    for(unsigned int tries=0;tries<MAX_READ_ATTEMPTS;tries++) {
        struct nova_pollfd poller={fd,NOVA_POLLIN|NOVA_POLLHUP|NOVA_POLLERR,0};
        struct nova_timespec pause={0,150000000};
        long ready=nova_poll(&poller,1U,&pause);
        if(ready==0 || ready==-NOVA_EINTR) continue;
        if(ready<0 || !(poller.revents&(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP))) return 0;
        struct nova_resource_broker_reply reply;
        struct nova_iovec iov;
        struct nova_msghdr msg;
        union {unsigned long align; unsigned char data[NOVA_CMSG_SPACE(sizeof(int))];} control;
        nova_zero(&reply,sizeof(reply)); nova_zero(&iov,sizeof(iov));
        nova_zero(&msg,sizeof(msg)); nova_zero(&control,sizeof(control));
        iov.iov_base=&reply; iov.iov_len=sizeof(reply);
        msg.msg_iov=&iov; msg.msg_iovlen=1;
        msg.msg_control=control.data; msg.msg_controllen=sizeof(control.data);
        long n=nova_recvmsg(fd,&msg,NOVA_MSG_CMSG_CLOEXEC);
        if(n==-NOVA_EAGAIN || n==-NOVA_EINTR) continue;
        if(n!=(long)sizeof(reply) || msg.msg_controllen!=0 ||
           (msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)) ||
           reply.magic!=NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC ||
           reply.version!=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION ||
           reply.size!=(unsigned int)sizeof(reply) || reply.request_id!=id ||
           reply.client_id!=11U || reply.process_id==0U ||
           !nova_streq(reply.app_id,"nova.resource.probe.idle")) return 0;
        if(malformed) return reply.opcode==NOVA_RESOURCE_BROKER_OP_ERROR &&
                             reply.result==NOVA_RESOURCE_BROKER_RESULT_BAD_REQUEST;
        return reply.opcode==NOVA_RESOURCE_BROKER_OP_SESSION &&
               reply.result==NOVA_RESOURCE_BROKER_RESULT_OK &&
               (reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_AUTHENTICATED) &&
               (reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_POLICY_BOUND) &&
               !(reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_FD_READONLY) &&
               reply.effective_capabilities==0ULL;
    }
    return 0;
}

static long seconds_now(void) {
    struct nova_timespec t={0,0};
    if(nova_clock_gettime(NOVA_CLOCK_MONOTONIC,&t)<0 || t.tv_sec<0) return -1;
    return t.tv_sec;
}

static void pause_briefly(void) {
    struct nova_timespec ts={0,200000000};
    (void)nova_syscall2(NOVA_NR_nanosleep,(long)&ts,0);
}

/* EOF and Linux's ECONNRESET are both socket closure. Ancillary length
 * is NOT meaningful on negative recvmsg(); do not reject reset errors.
 * Never send an extra HELLO to a potentially timed-out socket (SIGPIPE).
 */
static int is_closed_without_fd(int fd) {
    for(unsigned int k=0;k<MAX_READ_ATTEMPTS;k++) {
        struct nova_pollfd p={fd,NOVA_POLLIN|NOVA_POLLHUP|NOVA_POLLERR,0};
        struct nova_timespec t={0,150000000};
        long ready=nova_poll(&p,1U,&t);
        if(ready==0 || ready==-NOVA_EINTR) continue;
        if(ready<0 || !(p.revents&(NOVA_POLLIN|NOVA_POLLHUP|NOVA_POLLERR))) return 0;
        struct nova_resource_broker_reply r;
        struct nova_iovec iov;
        struct nova_msghdr msg;
        union {unsigned long align; unsigned char data[NOVA_CMSG_SPACE(sizeof(int))];} ctl;
        nova_zero(&r,sizeof(r)); nova_zero(&iov,sizeof(iov));
        nova_zero(&msg,sizeof(msg)); nova_zero(&ctl,sizeof(ctl));
        iov.iov_base=&r; iov.iov_len=sizeof(r);
        msg.msg_iov=&iov; msg.msg_iovlen=1;
        msg.msg_control=ctl.data; msg.msg_controllen=sizeof(ctl.data);
        long n=nova_recvmsg(fd,&msg,NOVA_MSG_CMSG_CLOEXEC);
        if(n==-NOVA_EAGAIN || n==-NOVA_EINTR) continue;
        if(n>0) return 0; /* Any response means expiry was not enforced. */
        return 1; /* No received message/ancillary FD, EOF or reset. */
    }
    return 0;
}

int nova_main(void) {
    int f[SESSIONS]={-1,-1,-1,-1};
    int replacement[2]={-1,-1};
    int ok=1;
    long started=-1;
    for(unsigned int i=0;i<SESSIONS;i++) {
        f[i]=connect_broker();
        if(f[i]<0 || !request_reply(f[i],100U+i,0)){ok=0;break;}
    }
    nova_putln(ok ? "[nova-step7-probe] four=PASS" : "[nova-step7-probe] four=FAIL");
    if(ok) {
        started=seconds_now();
        if(started<0) ok=0;
    }
    /* Delay uses guest CLOCK_MONOTONIC. 130s gives two idle connections
     * their full 120s lease plus broker's 1s poll scheduling margin.
     */
    unsigned int heartbeat=0;
    int invalid_sent=0;
    while(ok) {
        long now=seconds_now();
        if(now<started){ok=0;break;}
        unsigned long long elapsed=(unsigned long long)(now-started);
        if(elapsed>=130ULL) break;
        if(!invalid_sent && elapsed>=60ULL) {
            ok=request_reply(f[0],650U,1);
            invalid_sent=1;
            nova_putln(ok ? "[nova-step7-probe] malformed=PASS" : "[nova-step7-probe] malformed=FAIL");
            if(!ok) break;
        }
        if(heartbeat<HEARTBEATS && elapsed>=(unsigned long long)(heartbeat+1U)*20ULL) {
            ok=request_reply(f[2],200U+heartbeat,0) &&
               request_reply(f[3],300U+heartbeat,0);
            heartbeat++;
            if(!ok) break;
        }
        pause_briefly();
    }
    if(ok && (!invalid_sent || heartbeat!=HEARTBEATS)) ok=0;
    if(ok) {
        ok=is_closed_without_fd(f[0]) && is_closed_without_fd(f[1]);
        nova_putln(ok ? "[nova-step7-probe] idle=PASS" : "[nova-step7-probe] idle=FAIL");
    }
    if(ok) {
        ok=request_reply(f[2],810U,0) && request_reply(f[3],811U,0);
        nova_putln(ok ? "[nova-step7-probe] active=PASS" : "[nova-step7-probe] active=FAIL");
    }
    if(ok) {
        for(unsigned int i=0;i<2U;i++) {
            replacement[i]=connect_broker();
            if(replacement[i]<0 || !request_reply(replacement[i],900U+i,0)){ok=0;break;}
        }
        nova_putln(ok ? "[nova-step7-probe] reuse=PASS" : "[nova-step7-probe] reuse=FAIL");
    }
    for(unsigned int i=0;i<SESSIONS;i++) if(f[i]>=0) (void)nova_close(f[i]);
    for(unsigned int i=0;i<2U;i++) if(replacement[i]>=0) (void)nova_close(replacement[i]);
    nova_putln(ok ? "[nova-step7-probe] result=PASS" : "[nova-step7-probe] result=FAIL");
    return ok?0:1;
}
