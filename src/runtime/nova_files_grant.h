#ifndef NOVA_FILES_GRANT_H
#define NOVA_FILES_GRANT_H
#include "nova_runtime.h"
#if defined(__x86_64__)
#define NOVA_FILES_NR_SOCKETPAIR 53
#define NOVA_FILES_NR_DUP3 292
#define NOVA_FILES_NR_FCNTL 72
#define NOVA_FILES_NR_GETDENTS64 217L
#else
#define NOVA_FILES_NR_SOCKETPAIR 199
#define NOVA_FILES_NR_DUP3 24
#define NOVA_FILES_NR_FCNTL 25
#define NOVA_FILES_NR_GETDENTS64 61L
#endif
#define NOVA_FILES_NR_OPENAT2 437L
#define NOVA_FILES_GRANT_MAGIC 0x4e464447U
#define NOVA_FILES_GRANT_SOCKET_FD 3
struct nova_files_grant {unsigned int magic,version,client_id,scope;};
static inline int nova_files_directory_fd_valid(int fd) {
 struct nova_statx st;nova_zero(&st,sizeof(st));
 long flags=nova_syscall3(NOVA_FILES_NR_FCNTL,fd,3,0);
 if(flags<0||(flags&3)!=NOVA_O_RDONLY)return 0;
 if(nova_syscall5(NOVA_NR_statx,fd,(long)"",NOVA_AT_EMPTY_PATH,NOVA_STATX_TYPE,(long)&st)<0)return 0;
 return (st.stx_mask&NOVA_STATX_TYPE) && (st.stx_mode&NOVA_S_IFMT)==0040000;
}
static inline int nova_files_send_grant(int socket_fd,int directory_fd) {
 if(!nova_files_directory_fd_valid(directory_fd))return 0;
 struct nova_files_grant grant={NOVA_FILES_GRANT_MAGIC,1,3,1};
 struct nova_iovec iov={&grant,sizeof(grant)};struct nova_msghdr msg;
 union {struct nova_cmsghdr align;unsigned char bytes[NOVA_CMSG_SPACE(sizeof(int))];} control;
 nova_zero(&msg,sizeof(msg));nova_zero(&control,sizeof(control));
 msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control.bytes;msg.msg_controllen=sizeof(control);
 struct nova_cmsghdr *c=(struct nova_cmsghdr*)control.bytes;
 c->cmsg_len=NOVA_CMSG_LEN(sizeof(int));c->cmsg_level=NOVA_SOL_SOCKET;c->cmsg_type=NOVA_SCM_RIGHTS;
 nova_copy(control.bytes+NOVA_CMSG_DATA_OFFSET,&directory_fd,sizeof(directory_fd));
 return nova_sendmsg(socket_fd,&msg,0)==(long)sizeof(grant);
}
/* The socketpair is created by the trusted pre-exec novad child as uid 0.
 * It is never a pathname socket, and cannot be replaced by application input. */
static inline int nova_files_receive_grant(int socket_fd) {
 struct nova_ucred peer;unsigned int size=sizeof(peer);
 if(nova_getsockopt(socket_fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED,&peer,&size)<0 ||
    size!=sizeof(peer)||peer.uid!=0U||peer.pid!=(int)nova_getpid())return -1;
 struct nova_files_grant grant;struct nova_iovec iov={&grant,sizeof(grant)};struct nova_msghdr msg;
 union {struct nova_cmsghdr align;unsigned char bytes[NOVA_CMSG_SPACE(4*sizeof(int))];} control;
 nova_zero(&msg,sizeof(msg));nova_zero(&control,sizeof(control));nova_zero(&grant,sizeof(grant));
 msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control.bytes;msg.msg_controllen=sizeof(control);
 long got=nova_recvmsg(socket_fd,&msg,NOVA_MSG_TRUNC|0x40000000); /* MSG_CMSG_CLOEXEC */
 int fds[4]={-1,-1,-1,-1};unsigned int count=0;int valid=1;
 /* Ancillary buffer is bounded; parse all rights and close every fd on failure. */
 unsigned long pos=0;
 while(pos+sizeof(struct nova_cmsghdr)<=msg.msg_controllen){
  struct nova_cmsghdr *c=(struct nova_cmsghdr*)(control.bytes+pos);
  if(c->cmsg_len<NOVA_CMSG_DATA_OFFSET||c->cmsg_len>msg.msg_controllen-pos){valid=0;break;}
  unsigned long bytes=c->cmsg_len-NOVA_CMSG_DATA_OFFSET;
  if(c->cmsg_level!=NOVA_SOL_SOCKET||c->cmsg_type!=NOVA_SCM_RIGHTS||bytes%sizeof(int)){valid=0;break;}
  for(unsigned long j=0;j<bytes/sizeof(int);j++){
   int fd; nova_copy(&fd,control.bytes+pos+NOVA_CMSG_DATA_OFFSET+j*sizeof(int),sizeof(fd));
   if(count<4)fds[count++]=fd;else{nova_close(fd);valid=0;}
  }
  pos+=NOVA_CMSG_SPACE(bytes);
 }
 if(got!=(long)sizeof(grant)||msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)||count!=1||
    grant.magic!=NOVA_FILES_GRANT_MAGIC||grant.version!=1||grant.client_id!=3||grant.scope!=1)valid=0;
 if(valid&&!nova_files_directory_fd_valid(fds[0]))valid=0;
 if(!valid){for(unsigned int i=0;i<count;i++)nova_close(fds[i]);return -1;}
 return fds[0];
}
#endif
