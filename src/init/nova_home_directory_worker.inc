/* A private, launch-scoped metadata/directory capability worker. The app
 * remains uid 3; this isolated read-only worker uses the home owner uid 12004.
 * No pathname listener, arbitrary paths, writes, process creation or exec API. */
#include "nova_home_directory_protocol.h"
#ifndef NOVA_HOME_SANDBOX_SOURCE
#define NOVA_HOME_SANDBOX_SOURCE "/Data/App"
#endif
#if defined(__x86_64__)
#define NOVA_HOME_NR_SETSOCKOPT 54
#else
#define NOVA_HOME_NR_SETSOCKOPT 208
#endif
static int home_worker_policy(void){
 static struct nova_sock_filter f[]={
  NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARCH_OFFSET),
  NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_AUDIT_ARCH_NATIVE,1,0),
  NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_KILL_PROCESS),
  NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_NR_OFFSET),
#define HOME_ALLOW(nr) NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,nr,0,1),NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ALLOW)
  HOME_ALLOW(NOVA_NR_recvmsg),HOME_ALLOW(NOVA_NR_sendmsg),HOME_ALLOW(NOVA_NR_close),
  HOME_ALLOW(NOVA_FILES_NR_OPENAT2),HOME_ALLOW(NOVA_NR_statx),HOME_ALLOW(NOVA_NR_exit),
#undef HOME_ALLOW
  NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM)
 };
 struct nova_sock_fprog p={(unsigned short)(sizeof f/sizeof f[0]),f};
 return nova_prctl(NOVA_PR_SET_SECCOMP,NOVA_SECCOMP_MODE_FILTER,(unsigned long)&p,0,0)>=0;
}
static int home_worker_reply_bytes(int channel,const struct nova_home_directory_reply *reply,unsigned bytes,int pass){
 struct nova_iovec io={(void*)reply,bytes};struct nova_msghdr msg;
 union{struct nova_cmsghdr align;unsigned char bytes[NOVA_CMSG_SPACE(sizeof(int))];} control;
 nova_zero(&msg,sizeof msg);nova_zero(&control,sizeof control);msg.msg_iov=&io;msg.msg_iovlen=1;
 if(pass>=0){msg.msg_control=control.bytes;msg.msg_controllen=sizeof control;struct nova_cmsghdr*c=(void*)control.bytes;c->cmsg_len=NOVA_CMSG_LEN(sizeof(int));c->cmsg_level=NOVA_SOL_SOCKET;c->cmsg_type=NOVA_SCM_RIGHTS;nova_copy(control.bytes+NOVA_CMSG_DATA_OFFSET,&pass,sizeof pass);}
 return nova_sendmsg(channel,&msg,0x4000)==bytes;
}
static int home_worker_reply(int channel,const struct nova_home_directory_reply *reply,int pass){return home_worker_reply_bytes(channel,reply,sizeof *reply,pass);}
static void home_directory_worker(int channel,int root,int app_pid){
 for(int fd=0;fd<64;fd++)if(fd!=channel&&fd!=root)nova_close(fd);
 if(nova_setgroups(0,0)<0||nova_setgid(12004)<0||nova_setuid(12004)<0||nova_prctl(4 /* PR_SET_DUMPABLE */,0,0,0,0)<0||nova_prctl(NOVA_PR_SET_NO_NEW_PRIVS,1,0,0,0)<0||!home_worker_policy())nova_exit(126);
 int fds[NOVA_HOME_DIRECTORY_HANDLES];unsigned tokens[NOVA_HOME_DIRECTORY_HANDLES];
 for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++){fds[i]=-1;tokens[i]=0;}fds[0]=root;tokens[0]=1;
 struct nova_home_directory_reply ready={NOVA_HOME_DIRECTORY_MAGIC,1,0,0,0,1,0,0,0};if(!home_worker_reply(channel,&ready,-1))nova_exit(126);
 unsigned expected=1,next=2;
 for(;;){
  struct nova_home_directory_request req;struct nova_iovec io={&req,sizeof req};struct nova_msghdr msg;
  union{struct nova_cmsghdr align;unsigned char bytes[NOVA_CMSG_SPACE(sizeof(struct nova_ucred))+NOVA_CMSG_SPACE(4*sizeof(int))];} control;
  nova_zero(&msg,sizeof msg);nova_zero(&control,sizeof control);nova_zero(&req,sizeof req);msg.msg_iov=&io;msg.msg_iovlen=1;msg.msg_control=control.bytes;msg.msg_controllen=sizeof control;
  long got=nova_recvmsg(channel,&msg,NOVA_MSG_TRUNC|0x40000000);if(got==-4)continue;if(got<=0)break;
  unsigned credentials=0;int valid=1;unsigned long pos=0;
  while(pos+sizeof(struct nova_cmsghdr)<=msg.msg_controllen){
   struct nova_cmsghdr*c=(void*)(control.bytes+pos);if(c->cmsg_len<NOVA_CMSG_DATA_OFFSET||c->cmsg_len>msg.msg_controllen-pos){valid=0;break;}
   unsigned long bytes=c->cmsg_len-NOVA_CMSG_DATA_OFFSET;
   if(c->cmsg_level==NOVA_SOL_SOCKET&&c->cmsg_type==2&&bytes==sizeof(struct nova_ucred)){
    struct nova_ucred cred;nova_copy(&cred,control.bytes+pos+NOVA_CMSG_DATA_OFFSET,sizeof cred);credentials++;
    if(cred.pid!=app_pid||cred.uid!=3||cred.gid!=3)valid=0;
   }else{valid=0;if(c->cmsg_level==NOVA_SOL_SOCKET&&c->cmsg_type==NOVA_SCM_RIGHTS)for(unsigned long i=0;i+sizeof(int)<=bytes;i+=sizeof(int)){int fd;nova_copy(&fd,control.bytes+pos+NOVA_CMSG_DATA_OFFSET+i,sizeof fd);nova_close(fd);}}
   pos+=NOVA_CMSG_SPACE(bytes);
  }
  if(!valid||credentials!=1||msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)||got!=sizeof req||req.magic!=NOVA_HOME_DIRECTORY_MAGIC||req.version!=1||req.reserved||req.sequence!=expected||!expected)break;
  expected++;
  if(req.op<NOVA_HOME_DIRECTORY_OPEN||req.op>NOVA_HOME_DIRECTORY_DETAILS)break;
  if((req.op==NOVA_HOME_DIRECTORY_OPEN||req.op==NOVA_HOME_DIRECTORY_META||req.op==NOVA_HOME_DIRECTORY_PREVIEW||req.op==NOVA_HOME_DIRECTORY_DETAILS)&&!nova_home_component_valid(req.name))break;
  if(req.op==NOVA_HOME_DIRECTORY_CLOSE||req.op==NOVA_HOME_DIRECTORY_STAMP){for(unsigned i=0;i<sizeof req.name;i++)if(req.name[i])valid=0;if(!valid)break;}
  unsigned slot=NOVA_HOME_DIRECTORY_HANDLES;
  for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++)if(fds[i]>=0&&tokens[i]==req.handle){slot=i;break;}
  struct nova_home_directory_reply reply={NOVA_HOME_DIRECTORY_MAGIC,1,req.sequence,req.op,-9,0,0,0,0};int pass=-1;
  struct nova_home_directory_details_reply details;nova_zero(&details,sizeof details);
  if(slot<NOVA_HOME_DIRECTORY_HANDLES){
   if(req.op==NOVA_HOME_DIRECTORY_CLOSE){nova_close(fds[slot]);fds[slot]=-1;tokens[slot]=0;reply.status=0;}
   else if(req.op==NOVA_HOME_DIRECTORY_STAMP){
    struct nova_statx st;nova_zero(&st,sizeof st);reply.status=(int)nova_syscall5(NOVA_NR_statx,fds[slot],(long)"",NOVA_AT_EMPTY_PATH,0x40,(long)&st);
    if(reply.status>=0){if(!(st.stx_mask&0x40))reply.status=-5;else{reply.bytes=(unsigned long long)st.stx_mtime.tv_sec*1000000000ULL+st.stx_mtime.tv_nsec;reply.kind=4;reply.handle=NOVA_HOME_DIRECTORY_CAP_DETAILS;reply.status=0;}}
   }else{
    struct{unsigned long long flags,mode,resolve;}how={NOVA_O_CLOEXEC|NOVA_O_NOFOLLOW|(req.op==NOVA_HOME_DIRECTORY_OPEN?NOVA_O_RDONLY|0200000:req.op==NOVA_HOME_DIRECTORY_PREVIEW?NOVA_O_RDONLY|04000:010000000),0,0x0e};
    long child=nova_syscall4(NOVA_FILES_NR_OPENAT2,fds[slot],(long)req.name,(long)&how,sizeof how);reply.status=(int)child;
    if(child>=0&&req.op==NOVA_HOME_DIRECTORY_OPEN){
     unsigned free=NOVA_HOME_DIRECTORY_HANDLES;for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++)if(fds[i]<0){free=i;break;}
     if(free<NOVA_HOME_DIRECTORY_HANDLES&&next){fds[free]=(int)child;tokens[free]=next;reply.handle=next++;reply.status=0;pass=(int)child;}else{nova_close((int)child);reply.status=-24;}
    }else if(child>=0&&req.op==NOVA_HOME_DIRECTORY_PREVIEW){
     struct nova_statx st;nova_zero(&st,sizeof st);long rc=nova_syscall5(NOVA_NR_statx,child,(long)"",NOVA_AT_EMPTY_PATH,NOVA_STATX_TYPE|NOVA_STATX_SIZE,(long)&st);
     if(rc<0||(st.stx_mode&0170000)!=0100000||st.stx_size>4096){nova_close((int)child);reply.status=rc<0?(int)rc:-27;}
     else{reply.status=0;reply.kind=8;reply.bytes=st.stx_size;pass=(int)child;}
    }else if(child>=0){
     struct nova_statx st;nova_zero(&st,sizeof st);reply.status=(int)nova_syscall5(NOVA_NR_statx,child,(long)"",NOVA_AT_EMPTY_PATH,(req.op==NOVA_HOME_DIRECTORY_DETAILS?0x25f:NOVA_STATX_TYPE|NOVA_STATX_SIZE),(long)&st);nova_close((int)child);
     if(reply.status>=0){unsigned type=st.stx_mode&0170000;reply.kind=type==0100000?8:type==0040000?4:0;reply.bytes=st.stx_size;reply.status=reply.kind?0:-1;
      if(req.op==NOVA_HOME_DIRECTORY_DETAILS){
       if((st.stx_mask&0x21b)!=0x21b)reply.status=-95;
       else{details.mode=st.stx_mode&07777;details.uid=st.stx_uid;details.gid=st.stx_gid;details.links=(st.stx_mask&4)?st.stx_nlink:0;details.modified_seconds=(st.stx_mask&0x40)?st.stx_mtime.tv_sec:0;}
      }
     }
    }
   }
  }
  details.base=reply;int sent=req.op==NOVA_HOME_DIRECTORY_DETAILS?home_worker_reply_bytes(channel,&details.base,sizeof details,-1):home_worker_reply(channel,&reply,pass);
  if(req.op==NOVA_HOME_DIRECTORY_PREVIEW&&pass>=0)nova_close(pass);
  if(!sent)break;
 }
 for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++)if(fds[i]>=0)nova_close(fds[i]);nova_close(channel);nova_exit(0);
}
static int delegate_home_directory(void){
 int root=-1,broker[2]={-1,-1},grant[2]={-1,-1};long fd=nova_syscall4(NOVA_NR_openat,NOVA_AT_FDCWD,(long)NOVA_HOME_SANDBOX_SOURCE,NOVA_O_RDONLY|0200000|NOVA_O_NOFOLLOW|NOVA_O_CLOEXEC,0);if(fd<0)return 0;root=(int)fd;
 if(nova_syscall4(NOVA_FILES_NR_SOCKETPAIR,NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0,(long)broker)<0)goto fail;
 int yes=1;if(nova_syscall5(NOVA_HOME_NR_SETSOCKOPT,broker[0],NOVA_SOL_SOCKET,16,(long)&yes,sizeof yes)<0)goto fail;
 int app_pid=(int)nova_getpid();long child=nova_clone_process();if(child<0)goto fail;
 if(!child){
  long worker=nova_clone_process();if(worker<0)nova_exit(126);
  if(!worker)home_directory_worker(broker[0],root,app_pid);
  nova_exit(0); /* PID 1 adopts and reaps the worker even while Files lives. */
 }
 nova_close(broker[0]);broker[0]=-1;
 int bootstrap_status=0;long reaped;do{reaped=nova_wait4(child,&bootstrap_status,0);}while(reaped==-4);
 if(reaped!=child||bootstrap_status)goto fail;
 struct nova_pollfd ready_poll={broker[1],NOVA_POLLIN,0};struct nova_timespec wait={1,0};
 struct nova_home_directory_reply ready;
 if(nova_poll(&ready_poll,1,&wait)<=0||nova_read(broker[1],&ready,sizeof ready)!=sizeof ready||ready.magic!=NOVA_HOME_DIRECTORY_MAGIC||ready.version!=1||ready.sequence||ready.op||ready.status||ready.handle!=1)goto fail;
 if(nova_syscall4(NOVA_FILES_NR_SOCKETPAIR,NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0,(long)grant)<0)goto fail;
 struct nova_files_grant info={NOVA_FILES_GRANT_MAGIC,2,3,2};struct nova_iovec io={&info,sizeof info};struct nova_msghdr msg;
 union{struct nova_cmsghdr align;unsigned char bytes[NOVA_CMSG_SPACE(2*sizeof(int))];}control;
 nova_zero(&msg,sizeof msg);nova_zero(&control,sizeof control);msg.msg_iov=&io;msg.msg_iovlen=1;msg.msg_control=control.bytes;msg.msg_controllen=sizeof control;
 struct nova_cmsghdr*c=(void*)control.bytes;c->cmsg_len=NOVA_CMSG_LEN(2*sizeof(int));c->cmsg_level=NOVA_SOL_SOCKET;c->cmsg_type=NOVA_SCM_RIGHTS;int rights[2]={root,broker[1]};nova_copy(control.bytes+NOVA_CMSG_DATA_OFFSET,rights,sizeof rights);
 if(nova_sendmsg(grant[0],&msg,0)!=sizeof info)goto fail;
 nova_close(root);root=-1;nova_close(broker[1]);broker[1]=-1;
 if(nova_syscall3(NOVA_FILES_NR_DUP3,grant[1],NOVA_FILES_GRANT_SOCKET_FD,0)<0)goto fail;
 nova_close(grant[0]);nova_close(grant[1]);return 1;
fail:
 if(root>=0)nova_close(root);for(unsigned i=0;i<2;i++){if(broker[i]>=0)nova_close(broker[i]);if(grant[i]>=0)nova_close(grant[i]);}return 0;
}
