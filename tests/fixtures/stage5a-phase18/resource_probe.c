#include "nova_runtime.h"
#include "nova_resource_broker_protocol.h"

#ifndef NOVA_RESOURCE_PROBE_GRANTED
#error "Probe must be compiled in grant or deny mode"
#endif
#if NOVA_RESOURCE_PROBE_GRANTED
#define STATE_PATH "/Data/App/resource-grant.state"
#else
#define STATE_PATH "/Data/App/resource-deny.state"
#endif

static void unix_addr(struct nova_sockaddr_un *addr) {
    nova_zero(addr,sizeof(*addr)); addr->sun_family=NOVA_AF_UNIX;
    const char *src=NOVA_RESOURCE_BROKER_APP_SOCKET_PATH;
    for(unsigned long i=0;src[i] && i+1<sizeof(addr->sun_path);i++) addr->sun_path[i]=src[i];
}

static int test_one(unsigned int request_id,int expect_internal) {
    (void)expect_internal;
    struct nova_sockaddr_un addr;
    unix_addr(&addr);
    long socket_fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0);
    if(socket_fd<0) return 0;
    if(nova_connect((int)socket_fd,&addr,sizeof(addr))<0){ nova_close((int)socket_fd); return 0; }
    struct nova_resource_broker_request request;
    nova_zero(&request,sizeof(request));
    request.magic=NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC;
    request.version=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION;
    request.size=sizeof(request);
    request.opcode=NOVA_RESOURCE_BROKER_OP_OPEN_DEMO_RO;
    request.request_id=request_id;
    if(nova_write((int)socket_fd,&request,sizeof(request))!=(long)sizeof(request)){
        nova_close((int)socket_fd);return 0;
    }
    struct nova_resource_broker_reply reply;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    union { unsigned long align; unsigned char bytes[NOVA_CMSG_SPACE(sizeof(int))]; } control;
    nova_zero(&reply,sizeof(reply));nova_zero(&iov,sizeof(iov));
    nova_zero(&msg,sizeof(msg));nova_zero(&control,sizeof(control));
    iov.iov_base=&reply;iov.iov_len=sizeof(reply);
    msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control.bytes;
    msg.msg_controllen=sizeof(control.bytes);
    long n=nova_recvmsg((int)socket_fd,&msg,NOVA_MSG_CMSG_CLOEXEC);
    nova_close((int)socket_fd);
    if(n!=(long)sizeof(reply)||reply.magic!=NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC||
       reply.version!=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION||reply.request_id!=request_id||
       (msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC))) return 0;
#if NOVA_RESOURCE_PROBE_GRANTED
#ifdef NOVA_PHASE18_STEP3_ADVERSARIAL
    if(expect_internal) {
        /* Non-regular/oversized fixtures must never carry descriptors. */
        if(msg.msg_controllen!=0) {
            if(msg.msg_controllen>=NOVA_CMSG_LEN(sizeof(int))) {
                int leaked=-1;
                nova_copy(&leaked,control.bytes+NOVA_CMSG_DATA_OFFSET,sizeof(leaked));
                if(leaked>=0) (void)nova_close(leaked);
            }
            return 0;
        }
        return reply.opcode==NOVA_RESOURCE_BROKER_OP_ERROR &&
               reply.result==NOVA_RESOURCE_BROKER_RESULT_INTERNAL &&
               !(reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_FD_READONLY);
    }
#endif
    if(reply.opcode!=NOVA_RESOURCE_BROKER_OP_GRANT_FD||reply.result!=NOVA_RESOURCE_BROKER_RESULT_OK||
       !(reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_FD_READONLY)||
       (reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_NO_GRANT_SURFACE)) return 0;
    if(msg.msg_controllen<NOVA_CMSG_LEN(sizeof(int)) ||
       msg.msg_controllen>NOVA_CMSG_SPACE(sizeof(int))) return 0;
    const struct nova_cmsghdr *cmsg=(const struct nova_cmsghdr *)control.bytes;
    if(cmsg->cmsg_len!=NOVA_CMSG_LEN(sizeof(int))||cmsg->cmsg_level!=NOVA_SOL_SOCKET||
       cmsg->cmsg_type!=NOVA_SCM_RIGHTS) return 0;
    int fd=-1;nova_copy(&fd,control.bytes+NOVA_CMSG_DATA_OFFSET,sizeof(fd));
    if(fd<0) return 0;
    char contents[96];nova_zero(contents,sizeof(contents));
    long bytes=nova_read(fd,contents,sizeof(contents)-1);
    long denied=nova_write(fd,"X",1);
    nova_close(fd);
    return bytes>0 && nova_startswith(contents,"NovaOS Phase-18 Step-2:") && denied<0;
#else
    return reply.opcode==NOVA_RESOURCE_BROKER_OP_ERROR &&
           reply.result==NOVA_RESOURCE_BROKER_RESULT_DENIED &&
           msg.msg_controllen==0 &&
           !(reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_FD_READONLY);
#endif
}

int nova_main(void) {
    int success=0;
    /* Services may still be starting at first launch; bounded retry only.
     * This is a probe convenience, not an acceptance timeout override. */
    for(unsigned int attempt=0;attempt<20 && !success;attempt++){
        success=test_one(1U,0);
#if NOVA_RESOURCE_PROBE_GRANTED && defined(NOVA_PHASE18_STEP3_ADVERSARIAL)
        if(success){
            int directory=test_one(2U,1);
            if(directory) nova_putln("[nova-resource-probe] step3-directory=PASS");
            int fifo=test_one(3U,1);
            if(fifo) nova_putln("[nova-resource-probe] step3-fifo=PASS");
            int oversize=test_one(4U,1);
            if(oversize) nova_putln("[nova-resource-probe] step3-oversize=PASS");
            success=directory && fifo && oversize;
        }
#endif
        if(!success){ struct nova_timespec delay={0,100000000};
            (void)nova_syscall2(NOVA_NR_nanosleep,(long)&delay,0); }
    }
    long fd=nova_open_write(STATE_PATH,0600);
    if(fd>=0){
        nova_fd_putln((int)fd,success?"phase18_resource_probe=PASS":"phase18_resource_probe=FAIL");
        (void)nova_fsync((int)fd);(void)nova_close((int)fd);
    }
    nova_putln(success?"[nova-resource-probe] result=PASS":"[nova-resource-probe] result=FAIL");
    return success?0:1;
}
