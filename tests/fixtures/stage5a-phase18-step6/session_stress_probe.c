/* NovaOS Phase-18 Step-6 R2: acceptance-only session saturation/slot reuse.
 * One authenticated, unprivileged, sandboxed principal opens four separate
 * SEQPACKET sessions. No authorization shortcuts or kernel limits changed.
 */
#include "nova_runtime.h"
#include "nova_resource_broker_protocol.h"

#define STRESS_SOCKETS 4U
#define STRESS_READ_ATTEMPTS 20U
/* Linux AF_UNIX SOCK_SEQPACKET peer-reset errno, freestanding UAPI. */
#define STRESS_ECONNRESET 104L

static void addr_for_broker(struct nova_sockaddr_un *a) {
    const char *s=NOVA_RESOURCE_BROKER_APP_SOCKET_PATH;
    nova_zero(a,sizeof(*a)); a->sun_family=NOVA_AF_UNIX;
    for(unsigned long i=0U;s[i] && i+1U<sizeof(a->sun_path);i++)
        a->sun_path[i]=s[i];
}

static int dial(void) {
    struct nova_sockaddr_un addr;
    addr_for_broker(&addr);
    long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0);
    if(fd<0) return -1;
    if(nova_connect((int)fd,&addr,sizeof(addr))<0){ (void)nova_close((int)fd); return -1; }
    return (int)fd;
}

static int hello(int fd,unsigned int id) {
    struct nova_resource_broker_request req;
    nova_zero(&req,sizeof(req));
    req.magic=NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC;
    req.version=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION;
    req.size=(unsigned int)sizeof(req);
    req.opcode=NOVA_RESOURCE_BROKER_OP_HELLO;
    req.request_id=id;
    if(nova_write(fd,&req,sizeof(req))!=(long)sizeof(req)) return 0;

    for(unsigned int attempt=0U;attempt<STRESS_READ_ATTEMPTS;attempt++) {
        struct nova_pollfd poller={fd,NOVA_POLLIN|NOVA_POLLHUP|NOVA_POLLERR,0};
        struct nova_timespec timeout={0,150000000};
        long ready=nova_poll(&poller,1U,&timeout);
        if(ready==-NOVA_EINTR || ready==0) continue;
        if(ready<0 || !(poller.revents&(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP))) return 0;
        struct nova_resource_broker_reply reply;
        struct nova_iovec iov;
        struct nova_msghdr msg;
        union {unsigned long align;unsigned char data[NOVA_CMSG_SPACE(sizeof(int))];} control;
        nova_zero(&reply,sizeof(reply));nova_zero(&iov,sizeof(iov));
        nova_zero(&msg,sizeof(msg));nova_zero(&control,sizeof(control));
        iov.iov_base=&reply;iov.iov_len=sizeof(reply);
        msg.msg_iov=&iov;msg.msg_iovlen=1;
        msg.msg_control=control.data;msg.msg_controllen=sizeof(control.data);
        long received=nova_recvmsg(fd,&msg,NOVA_MSG_CMSG_CLOEXEC);
        if(received==-NOVA_EAGAIN || received==-NOVA_EINTR) continue;
        /* HELLO must NEVER transfer an SCM_RIGHTS descriptor. */
        return received==(long)sizeof(reply) && msg.msg_controllen==0 &&
               !(msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)) &&
               reply.magic==NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC &&
               reply.version==NOVA_RESOURCE_BROKER_PROTOCOL_VERSION &&
               reply.size==(unsigned int)sizeof(reply) &&
               reply.request_id==id &&
               reply.opcode==NOVA_RESOURCE_BROKER_OP_SESSION &&
               reply.result==NOVA_RESOURCE_BROKER_RESULT_OK &&
               (reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_AUTHENTICATED) &&
               (reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_POLICY_BOUND) &&
               (reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_NO_GRANT_SURFACE) &&
               !(reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_FD_READONLY) &&
               reply.client_id==11U && reply.process_id!=0U &&
               reply.app_generation!=0ULL && reply.policy_generation!=0ULL &&
               reply.effective_capabilities==0ULL &&
               nova_streq(reply.app_id,"nova.resource.probe.stress");
    }
    return 0;
}

static int rejected_no_fd(int fd) {
    struct nova_resource_broker_request req;
    nova_zero(&req,sizeof(req));
    req.magic=NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC;
    req.version=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION;
    req.opcode=NOVA_RESOURCE_BROKER_OP_HELLO;
    req.size=(unsigned int)sizeof(req);
    req.request_id=600U;
    /* Rejection may race with the send: the client must observe EOF/error,
       never a successful reply or ancillary descriptor. */
    (void)nova_write(fd,&req,sizeof(req));
    for(unsigned int i=0U;i<STRESS_READ_ATTEMPTS;i++) {
        struct nova_pollfd p={fd,NOVA_POLLIN|NOVA_POLLHUP|NOVA_POLLERR,0};
        struct nova_timespec t={0,150000000};
        long ready=nova_poll(&p,1U,&t);
        if(ready==-NOVA_EINTR || ready==0) continue;
        if(ready<0) return 0;
        if(!(p.revents&(NOVA_POLLIN|NOVA_POLLHUP|NOVA_POLLERR))) continue;
        struct nova_resource_broker_reply reply;
        struct nova_iovec iov;
        struct nova_msghdr msg;
        union {unsigned long align;unsigned char data[NOVA_CMSG_SPACE(sizeof(int))];} control;
        nova_zero(&reply,sizeof(reply));nova_zero(&iov,sizeof(iov));
        nova_zero(&msg,sizeof(msg));nova_zero(&control,sizeof(control));
        iov.iov_base=&reply;iov.iov_len=sizeof(reply);
        msg.msg_iov=&iov;msg.msg_iovlen=1;
        msg.msg_control=control.data;msg.msg_controllen=sizeof(control.data);
        long n=nova_recvmsg(fd,&msg,NOVA_MSG_CMSG_CLOEXEC);
        if(n==-NOVA_EINTR || n==-NOVA_EAGAIN) continue;
        /* If the broker closes an accepted socket with an unread HELLO,
         * recvmsg may return -ECONNRESET. On syscall failure the input
         * msg_controllen is NOT a count of received ancillary data.
         * Accept only this specific reset or a clean ancillary-free EOF.
         * All positive replies and unrelated errors are forbidden. */
        if(n==-STRESS_ECONNRESET){
            nova_putln("[nova-step6-probe] fifth_reset=EXPECTED");
            return 1;
        }
        if(n>0) nova_putln("[nova-step6-probe] fifth_reply=UNEXPECTED");
        else if(n<0) nova_putln("[nova-step6-probe] fifth_errno=UNEXPECTED");
        return n==0 && msg.msg_controllen==0 &&
               !(msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC));
    }
    return 0;
}

static void short_delay(void) {
    struct nova_timespec ts={0,150000000};
    (void)nova_syscall2(NOVA_NR_nanosleep,(long)&ts,0);
}

int nova_main(void) {
    int fd[STRESS_SOCKETS];
    for(unsigned int i=0U;i<STRESS_SOCKETS;i++) fd[i]=-1;
    int ok=1;
    for(unsigned int i=0U;i<STRESS_SOCKETS;i++){
        fd[i]=dial();
        if(fd[i]<0 || !hello(fd[i],100U+i)){ok=0;break;}
    }
    nova_putln(ok ? "[nova-step6-probe] four=PASS" : "[nova-step6-probe] four=FAIL");
    if(ok){
        /* An existing live session remains serviceable even at saturation. */
        ok=hello(fd[0],200U);
        nova_putln(ok ? "[nova-step6-probe] fairness=PASS" : "[nova-step6-probe] fairness=FAIL");
    }
    if(ok){
        int extra=dial();
        if(extra<0) nova_putln("[nova-step6-probe] fifth_connect=FAIL");
        ok=extra>=0 && rejected_no_fd(extra);
        if(extra>=0) (void)nova_close(extra);
        nova_putln(ok ? "[nova-step6-probe] fifth=PASS" : "[nova-step6-probe] fifth=FAIL");
    }
    if(ok){
        (void)nova_close(fd[0]);fd[0]=-1;
        /* Wait for peer HUP to release the broker slot; retry only boundedly.
         * Each failed retry must be closed before making the next attempt. */
        int reused=0;
        for(unsigned int i=0U;i<STRESS_READ_ATTEMPTS && !reused;i++){
            short_delay();
            int attempt=dial();
            if(attempt<0) continue;
            if(hello(attempt,300U+i)){
                reused=1;
                fd[0]=attempt;
            } else (void)nova_close(attempt);
        }
        ok=reused;
        nova_putln(ok ? "[nova-step6-probe] reuse=PASS" : "[nova-step6-probe] reuse=FAIL");
    }
    for(unsigned int i=0U;i<STRESS_SOCKETS;i++)
        if(fd[i]>=0) (void)nova_close(fd[i]);
    nova_putln(ok ? "[nova-step6-probe] result=PASS" : "[nova-step6-probe] result=FAIL");
    return ok?0:1;
}
