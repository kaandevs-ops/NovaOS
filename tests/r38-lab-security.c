#define _GNU_SOURCE
#include <assert.h>
#define main unused_launcher
#include "../src/terminal/nova_terminal_session.c"
#undef main
int main(int argc,char**argv){
 assert(argc==3&&getuid()==0);int mode=atoi(argv[1]);int fd=open(argv[2],O_CREAT|O_EXCL|O_RDONLY,0600);assert(fd>=0&&fchown(fd,12004,12004)==0);
 int s[2];assert(socketpair(AF_UNIX,SOCK_SEQPACKET,0,s)==0);pid_t worker=fork();assert(worker>=0);
 if(!worker){close(s[0]);lab_worker(s[1],fd);}
 close(s[1]);struct nova_lab_message ready;assert(read(s[0],&ready,sizeof ready)==sizeof ready&&ready.magic==NOVA_LAB_MAGIC);
 pid_t client=fork();assert(client>=0);
 if(!client){
  unsigned uid=mode==1?12006:mode==2?0:12004;if(uid)assert(!setgroups(0,NULL)&&!setgid(uid)&&!setuid(uid));
  struct nova_lab_message m={.magic=NOVA_LAB_MAGIC,.version=1,.tag=1,.uid=12006,.gid=12006};
  if(mode==3)m.uid=m.gid=0;if(mode==4)m.version=99;if(mode==5)m.reserved=1;
  if(mode==6){char ctrl[CMSG_SPACE(sizeof(int))]={0};struct iovec io={&m,sizeof m};struct msghdr msg={.msg_iov=&io,.msg_iovlen=1,.msg_control=ctrl,.msg_controllen=sizeof ctrl};struct cmsghdr*c=CMSG_FIRSTHDR(&msg);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&fd,sizeof fd);assert(sendmsg(s[0],&msg,MSG_NOSIGNAL)==sizeof m);}
  else assert(write(s[0],&m,sizeof m)==sizeof m);
  ssize_t n=read(s[0],&m,sizeof m);if(mode==0)assert(n==sizeof m&&m.status==0);else if(mode==3)assert(n==sizeof m&&m.status==-EPERM);else assert(n==0);
  close(s[0]);_exit(0);
 }
 close(s[0]);int status;assert(waitpid(client,&status,0)==client&&status==0);assert(waitpid(worker,&status,0)==worker&&status==0);
 struct stat st;assert(!fstat(fd,&st)&&st.st_uid==(mode==0?12006:12004));close(fd);puts("LAB_AUTHORITY_CHECK_PASS");return 0;
}
