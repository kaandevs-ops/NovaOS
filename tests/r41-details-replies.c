#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "nova_files_guest_metadata.h"
#include "nova_files_home_client.h"
static unsigned fd_count(void){DIR*d=opendir("/proc/self/fd");assert(d);unsigned n=0;while(readdir(d))n++;closedir(d);return n;}
/* Test only the parser behind an already established channel; this fixture
 * deliberately does NOT pretend to authenticate a fake peer as a root grant. */
int main(void){
 unsigned before=fd_count();
 for(unsigned mode=0;mode<14;mode++){
  int sockets[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET,0,sockets));pid_t pid=fork();assert(pid>=0);
  if(!pid){close(sockets[0]);struct nova_home_directory_request req;assert(recv(sockets[1],&req,sizeof req,0)==sizeof req&&req.op==(mode>=12?NOVA_HOME_DIRECTORY_STAMP:NOVA_HOME_DIRECTORY_DETAILS));
   struct {struct nova_home_directory_details_reply reply;unsigned char extra;}packet={0};struct nova_home_directory_details_reply*d=&packet.reply;d->base=(struct nova_home_directory_reply){NOVA_HOME_DIRECTORY_MAGIC,1,req.sequence,req.op,0,0,12,8,0};d->mode=0600;d->uid=~0U;d->gid=4294967294U;
   size_t bytes=sizeof *d;
   if(mode>=12){bytes=40;d->base.kind=mode==12?4:8;d->base.handle=mode==12?2:0;}
   if(mode==1)bytes=40;if(mode==2)bytes=65;if(mode==4)d->base.reserved=1;if(mode==5)d->base.status=1;if(mode==6)d->mode=0100600;if(mode==7)d->base.sequence++;if(mode==8)d->base.version=99;if(mode==9)d->base.kind=4;if(mode==10)d->base.handle=1;if(mode==11)d->base.status=-13;
   struct iovec io={&packet,bytes};struct msghdr msg={0};msg.msg_iov=&io;msg.msg_iovlen=1;unsigned char control[CMSG_SPACE(sizeof(int))]={0};int pass=-1;
   if(mode==3){pass=open("/dev/null",O_RDONLY);assert(pass>=0);msg.msg_control=control;msg.msg_controllen=sizeof control;struct cmsghdr*c=CMSG_FIRSTHDR(&msg);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&pass,sizeof pass);}
   assert(sendmsg(sockets[1],&msg,0)==(ssize_t)bytes);if(pass>=0)close(pass);close(sockets[1]);_exit(0);
  }
  close(sockets[1]);home_client.shared=1;home_client.details=mode>=12?-1:1;home_client.socket=sockets[0];home_client.sequence=0;for(unsigned i=0;i<NOVA_HOME_DIRECTORY_HANDLES;i++){home_client.fds[i]=-1;home_client.tokens[i]=0;}home_client.fds[0]=123;home_client.tokens[0]=1;
  struct nova_files_metadata out;memset(&out,0x73,sizeof out);int rc=nova_files_home_metadata(123,"note.txt",8,&out);
  if(mode==0)assert(rc&&out.valid&&out.posix_valid&&out.uid==~0U&&out.gid==4294967294U&&out.mode==0600&&out.bytes==12);
  else{assert(!rc&&!out.valid&&!out.posix_valid);if(mode!=9&&mode!=11)assert(home_client.socket<0);}
  home_client_failed();int status;assert(waitpid(pid,&status,0)==pid&&status==0);assert(fd_count()==before);
 }
 puts("PASS 14 real seqpacket details reply cases, malformed size/header/sequence/mode/handle, unexpected SCM_RIGHTS cleanup and full UID/GID width");
}
