/* Step-4 acceptance-only: retain the existing Step-2 grant/deny probe. */
#define nova_main phase18_legacy_probe_main
#include "resource_probe.c"
#undef nova_main

/* Each transaction uses a fresh authenticated broker socket. */
static int catalog_case(unsigned int serial,unsigned int op,unsigned int id,
                        unsigned int extra,unsigned int expected,const char *prefix) {
    struct nova_sockaddr_un addr;
    unix_addr(&addr);
    long s=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0);
    if(s<0) return 0;
    if(nova_connect((int)s,&addr,sizeof(addr))<0){nova_close((int)s);return 0;}
    struct nova_resource_broker_request req;
    nova_zero(&req,sizeof(req));
    req.magic=NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC;
    req.version=NOVA_RESOURCE_BROKER_PROTOCOL_VERSION;
    req.opcode=op;req.size=sizeof(req);req.request_id=serial;
    req.reserved0=id;req.reserved1=extra;
    if(nova_write((int)s,&req,sizeof(req))!=(long)sizeof(req)){
        nova_close((int)s);return 0;
    }
    struct nova_resource_broker_reply reply;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    union {unsigned long align;unsigned char b[NOVA_CMSG_SPACE(sizeof(int))];} ancillary;
    nova_zero(&reply,sizeof(reply));nova_zero(&iov,sizeof(iov));
    nova_zero(&msg,sizeof(msg));nova_zero(&ancillary,sizeof(ancillary));
    iov.iov_base=&reply;iov.iov_len=sizeof(reply);
    msg.msg_iov=&iov;msg.msg_iovlen=1;
    msg.msg_control=ancillary.b;msg.msg_controllen=sizeof(ancillary.b);
    long n=nova_recvmsg((int)s,&msg,NOVA_MSG_CMSG_CLOEXEC);
    nova_close((int)s);
    int fd=-1;
    if(msg.msg_controllen>=NOVA_CMSG_LEN(sizeof(int)))
        nova_copy(&fd,ancillary.b+NOVA_CMSG_DATA_OFFSET,sizeof(fd));
    int ok=n==(long)sizeof(reply) && reply.magic==NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC &&
           reply.version==NOVA_RESOURCE_BROKER_PROTOCOL_VERSION &&
           reply.request_id==serial && reply.result==expected &&
           !(msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC));
    if(prefix) {
        struct nova_cmsghdr *c=(struct nova_cmsghdr *)ancillary.b;
        ok=ok && reply.opcode==NOVA_RESOURCE_BROKER_OP_GRANT_FD &&
           (reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_FD_READONLY) &&
           !(reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_NO_GRANT_SURFACE) &&
           msg.msg_controllen==NOVA_CMSG_SPACE(sizeof(int)) &&
           c->cmsg_len==NOVA_CMSG_LEN(sizeof(int)) &&
           c->cmsg_level==NOVA_SOL_SOCKET && c->cmsg_type==NOVA_SCM_RIGHTS && fd>=0;
        if(ok){
            char content[128];nova_zero(content,sizeof(content));
            long count=nova_read(fd,content,sizeof(content)-1);
            long written=nova_write(fd,"X",1);
            ok=count>0 && nova_startswith(content,prefix) && written<0;
        }
    } else {
        ok=ok && reply.opcode==NOVA_RESOURCE_BROKER_OP_ERROR &&
           msg.msg_controllen==0 && fd<0 &&
           !(reply.broker_flags&NOVA_RESOURCE_BROKER_FLAG_FD_READONLY);
    }
    if(fd>=0) nova_close(fd);
    return ok;
}

int nova_main(void) {
    int legacy=phase18_legacy_probe_main();
    if(legacy!=0){nova_putln("[nova-step4-probe] result=FAIL");return 1;}
#if NOVA_RESOURCE_PROBE_GRANTED
    int a=catalog_case(101U,NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO,
        NOVA_RESOURCE_ID_DEMO,0U,NOVA_RESOURCE_BROKER_RESULT_OK,"NovaOS Phase-18 Step-2:");
    if(a) nova_putln("[nova-step4-probe] id1=PASS");
    int b=catalog_case(102U,NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO,
        NOVA_RESOURCE_ID_NOTES,0U,NOVA_RESOURCE_BROKER_RESULT_OK,"NovaOS Phase-18 Step-4:");
    if(b) nova_putln("[nova-step4-probe] id2=PASS");
    int u=catalog_case(103U,NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO,
        0xffffffffU,0U,NOVA_RESOURCE_BROKER_RESULT_DENIED,0);
    if(u) nova_putln("[nova-step4-probe] unknown=PASS");
    int z=catalog_case(104U,NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO,
        0U,0U,NOVA_RESOURCE_BROKER_RESULT_BAD_REQUEST,0);
    if(z) nova_putln("[nova-step4-probe] zero=PASS");
    int r=catalog_case(105U,NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO,
        NOVA_RESOURCE_ID_DEMO,1U,NOVA_RESOURCE_BROKER_RESULT_BAD_REQUEST,0);
    if(r) nova_putln("[nova-step4-probe] reserved=PASS");
    int l=catalog_case(106U,NOVA_RESOURCE_BROKER_OP_OPEN_DEMO_RO,
        NOVA_RESOURCE_ID_NOTES,0U,NOVA_RESOURCE_BROKER_RESULT_BAD_REQUEST,0);
    if(l) nova_putln("[nova-step4-probe] legacy-bad=PASS");
    int passed=a&&b&&u&&z&&r&&l;
#else
    int a=catalog_case(201U,NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO,
        NOVA_RESOURCE_ID_DEMO,0U,NOVA_RESOURCE_BROKER_RESULT_DENIED,0);
    if(a) nova_putln("[nova-step4-probe] unauthorized-id1=PASS");
    int b=catalog_case(202U,NOVA_RESOURCE_BROKER_OP_OPEN_RESOURCE_RO,
        NOVA_RESOURCE_ID_NOTES,0U,NOVA_RESOURCE_BROKER_RESULT_DENIED,0);
    if(b) nova_putln("[nova-step4-probe] unauthorized-id2=PASS");
    int passed=a&&b;
#endif
    nova_putln(passed?"[nova-step4-probe] result=PASS":"[nova-step4-probe] result=FAIL");
    return passed?0:1;
}
