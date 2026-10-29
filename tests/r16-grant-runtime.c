#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "nova_files_grant.h"
static int fd_count(void){int n=0;for(int i=0;i<1024;i++)if(fcntl(i,F_GETFD)>=0)n++;return n;}
static void delivery(int dir,int invalid,int count,int extra){
 int pair[2];assert(socketpair(AF_UNIX,SOCK_SEQPACKET,0,pair)==0);
 struct {struct nova_files_grant grant;unsigned int extra;} p={{NOVA_FILES_GRANT_MAGIC,1,3,1},0};
 if(invalid==1)p.grant.client_id=4;
 if(invalid==2)p.grant.scope=2;
 if(invalid==3)p.grant.version=2;
 if(invalid==4)p.grant.magic=0;
 int fds[5]={dir,dir,dir,dir,dir};char control[CMSG_SPACE(sizeof(fds))];
 struct iovec iov={&p,sizeof(p.grant)+(extra?4:0)};struct msghdr msg={0};
 msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control;msg.msg_controllen=CMSG_SPACE((unsigned long)count*sizeof(int));
 struct cmsghdr *c=CMSG_FIRSTHDR(&msg);c->cmsg_len=CMSG_LEN((unsigned long)count*sizeof(int));c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;
 nova_copy(CMSG_DATA(c),fds,(unsigned long)count*sizeof(int));
 assert(sendmsg(pair[0],&msg,0)>0);int before=fd_count();int received=nova_files_receive_grant(pair[1]);
 if(!invalid&&count==1&&!extra&&nova_files_directory_fd_valid(dir)){
  assert(received>=0);assert(fcntl(received,F_GETFD)&FD_CLOEXEC);close(received);
 }else assert(received<0);
 assert(before==fd_count());close(pair[0]);close(pair[1]);
}
int main(void){
 assert(getuid()==0);char path[]="/tmp/nova-r16-grant-XXXXXX";assert(mkdtemp(path));
 int dir=open(path,O_RDONLY|O_DIRECTORY);assert(dir>=0);
 delivery(dir,0,1,0);for(int i=1;i<=4;i++)delivery(dir,i,1,0);
 delivery(dir,0,2,0);delivery(dir,0,5,0);delivery(dir,0,1,1);
 int file=openat(dir,"regular",O_CREAT|O_RDWR,0600);assert(file>=0);delivery(file,0,1,0);
 int ro=openat(dir,"regular",O_RDONLY);assert(ro>=0);delivery(ro,0,1,0);
 int pair[2];assert(socketpair(AF_UNIX,SOCK_SEQPACKET,0,pair)==0);
 assert(nova_files_send_grant(pair[0],dir));pid_t child=fork();assert(child>=0);
 if(!child){assert(nova_files_receive_grant(pair[1])<0);_exit(0);}
 int status;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
 close(pair[0]);close(pair[1]);close(ro);close(file);close(dir);
 puts("R16_GRANT_ID_SCOPE_VERSION_TYPE_COUNT_TRUNCATION_PEER_PID_CLOEXEC_FD_CLEANUP=PASS");return 0;
}
