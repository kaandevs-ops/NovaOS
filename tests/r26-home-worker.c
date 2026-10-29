#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#define nova_main unused_novad_entry
#include "../src/init/main.c"
#undef nova_main
#include "../files/src/nova_files_guest_adapter.h"
#include "../files/src/nova_files_guest_metadata.h"
#include "../files/src/nova_files_home_client.h"
static int count_fds(void){
 struct{unsigned long long flags,mode,resolve;}how={NOVA_O_RDONLY|0200000,0,0};
 long fd=nova_syscall4(NOVA_FILES_NR_OPENAT2,NOVA_AT_FDCWD,(long)"/proc/self/fd",(long)&how,sizeof how);assert(fd>=0);
 unsigned char b[2048];long n=nova_syscall3(NOVA_FILES_NR_GETDENTS64,fd,(long)b,sizeof b);assert(n>=0);unsigned pos=0,count=0;
 while(pos<(unsigned)n){unsigned short len;memcpy(&len,b+pos+16,2);assert(len>=20&&pos+len<=(unsigned)n);count++;pos+=len;}nova_close((int)fd);return count;
}
static void exercise(int mode){
 assert(nova_close_range(3,~0U,0)>=0);assert(delegate_home_directory());
 assert(nova_setgroups(0,0)>=0&&nova_setgid(3)>=0&&nova_setuid(mode==5?12005:3)>=0);
 int root=nova_files_receive_home_grant(3);assert(root>=0&&home_client.shared);nova_close(3);
 assert(nova_prctl(NOVA_PR_SET_NO_NEW_PRIVS,1,0,0,0)>=0);
 if(mode!=4)assert(sandbox_install_files_seccomp());
 struct nova_files_metadata meta;
 if(mode==4){long pid=nova_clone_process();assert(pid>=0);if(!pid){int fd=-1;struct nova_home_directory_reply reply;assert(home_request(NOVA_HOME_DIRECTORY_META,1,"note.txt",&reply,&fd)<0);nova_exit(0);}int status=0;assert(nova_wait4(pid,&status,0)==pid&&status==0);assert(!nova_files_home_metadata(root,"note.txt",8,&meta));nova_exit(0);}
 if(mode==5){struct nova_files_metadata bad;assert(!nova_files_home_metadata(root,"note.txt",8,&bad));nova_exit(0);}
 if(mode==6||mode==7){struct nova_home_directory_reply reply;int fd=-1;assert(home_request(mode==6?NOVA_HOME_DIRECTORY_STAMP:99,1,"note.txt",&reply,&fd)<0);nova_exit(0);}
 if(mode==1){struct nova_home_directory_reply reply;int fd=-1;assert(home_request(NOVA_HOME_DIRECTORY_META,1,"..",&reply,&fd)<0);nova_exit(0);}
 if(mode==2){struct nova_home_directory_request req={.magic=NOVA_HOME_DIRECTORY_MAGIC,.version=1,.sequence=9,.op=2,.handle=1};memcpy(req.name,"note.txt",9);assert(nova_write(home_client.socket,&req,sizeof req)==sizeof req);struct nova_pollfd f={home_client.socket,NOVA_POLLIN,0};struct nova_timespec t={1,0};assert(nova_poll(&f,1,&t)>0);char b[64];assert(nova_read(home_client.socket,b,sizeof b)<=0);nova_exit(0);}
 if(mode==3){struct nova_home_directory_request req={.magic=NOVA_HOME_DIRECTORY_MAGIC,.version=1,.sequence=1,.op=2,.handle=1};memcpy(req.name,"note.txt",9);struct nova_iovec io={&req,sizeof req};struct nova_msghdr msg;union{struct nova_cmsghdr a;unsigned char b[NOVA_CMSG_SPACE(sizeof(int))];}control;nova_zero(&msg,sizeof msg);nova_zero(&control,sizeof control);msg.msg_iov=&io;msg.msg_iovlen=1;msg.msg_control=control.b;msg.msg_controllen=sizeof control;struct nova_cmsghdr*c=(void*)control.b;c->cmsg_len=NOVA_CMSG_LEN(sizeof(int));c->cmsg_level=NOVA_SOL_SOCKET;c->cmsg_type=NOVA_SCM_RIGHTS;nova_copy(control.b+NOVA_CMSG_DATA_OFFSET,&root,sizeof root);assert(nova_sendmsg(home_client.socket,&msg,0)==sizeof req);struct nova_pollfd f={home_client.socket,NOVA_POLLIN,0};struct nova_timespec t={1,0};assert(nova_poll(&f,1,&t)>0);char b[64];assert(nova_read(home_client.socket,b,sizeof b)<=0);nova_exit(0);}
 assert(nova_files_guest_open(root,"Documents",0)==-13); /* DAC unchanged */
 struct nova_home_directory_reply stamp;int stamp_fd=-1;assert(!home_request(NOVA_HOME_DIRECTORY_STAMP,1,0,&stamp,&stamp_fd)&&stamp.kind==4&&stamp.bytes>0);unsigned long long initial_stamp=stamp.bytes;assert(!home_request(NOVA_HOME_DIRECTORY_STAMP,1,0,&stamp,&stamp_fd)&&stamp.bytes==initial_stamp);
 int before=count_fds();struct nova_files_model model;assert(!nova_files_init(&model,root,&nova_files_home_ops,0));assert(!nova_files_list(&model));assert(model.count>=3);
 assert(nova_files_home_metadata(root,"note.txt",8,&meta)&&meta.bytes==12);
 assert(!nova_files_home_metadata(root,"escape",4,&meta));assert(home_guest_open(root,"escape",0)<0);
 int fd=-1;struct nova_home_directory_reply reply;assert(home_request(NOVA_HOME_DIRECTORY_META,0xffffffffU,"note.txt",&reply,&fd)==-9);
 for(unsigned i=0;i<1000;i++){
  assert(!nova_files_enter(&model,"Documents"));assert(!nova_files_list(&model));assert(nova_files_home_metadata(model.directory_fds[model.depth],"inside.txt",8,&meta)&&meta.bytes==7);
  assert(!nova_files_parent(&model));
 }
 int caps[31];unsigned handles[31];
 for(unsigned i=0;i<31;i++){assert(home_request(NOVA_HOME_DIRECTORY_OPEN,1,"Documents",&reply,&caps[i])==0);handles[i]=reply.handle;}
 assert(home_request(NOVA_HOME_DIRECTORY_OPEN,1,"Documents",&reply,&fd)==-24);
 for(unsigned i=0;i<31;i++){assert(home_request(NOVA_HOME_DIRECTORY_CLOSE,handles[i],0,&reply,&fd)==0);nova_close(caps[i]);}
 assert(home_request(NOVA_HOME_DIRECTORY_META,handles[0],"inside.txt",&reply,&fd)==-9);
 assert(count_fds()==before);assert(nova_files_home_metadata(root,"note.txt",8,&meta));nova_files_close(&model);assert(home_client.socket<0);nova_putln("R26_HOME_WORKER_POSITIVE_READONLY_DAC_AND_1000_CYCLES=PASS");nova_exit(0);
}
int main(int argc,char**argv){assert(argc==2&&getuid()==0);assert(!prctl(PR_SET_CHILD_SUBREAPER,1));int mode=atoi(argv[1]);pid_t child=fork();assert(child>=0);if(!child)exercise(mode);int status=0;assert(waitpid(child,&status,0)==child);assert(status==0);unsigned reaped=0;for(unsigned i=0;i<200;i++){int s;pid_t p=waitpid(-1,&s,WNOHANG);if(p>0){assert(s==0);reaped++;continue;}if(p<0&&errno==ECHILD)break;usleep(10000);}assert(reaped==1);puts("PASS: private worker reaped after app close/crash/invalid caller");}
