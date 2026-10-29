#ifndef NOVA_TERM_ROOT
#define NOVA_TERM_ROOT "/run/nova/software-root"
#endif
#define main nova_terminal_reference_main
#include "nova_terminal_session.c"
#undef main
#include "../runtime/nova_software_protocol.h"
struct sw_app{pid_t pid;int fd;unsigned used;unsigned char input[4096];};
static int sw_exec(unsigned op,unsigned index){
 pid_t p=fork();if(p<0)return 4;if(!p){int d=open("/dev/null",O_RDWR);if(d<0)_exit(4);for(int i=0;i<3;i++)if(dup2(d,i)<0)_exit(4);if(close_range(3,~0U,0)||term_install_policy())_exit(4);
 char*args[]={"nova-pkg",op==NOVA_SW_LIST?"status":op==NOVA_SW_INSTALL?"install":"remove",(char*)nova_sw_ids[index],op==NOVA_SW_LIST?"--machine":NULL,NULL};char*env[]={"HOME=/home/nova","PATH=/bin","LANG=C",NULL};execve("/bin/nova-pkg",args,env);_exit(4);}
 int st;pid_t got;do got=waitpid(p,&st,0);while(got<0&&errno==EINTR);return got==p&&WIFEXITED(st)?WEXITSTATUS(st):4;
}
static unsigned sw_mask(struct sw_app*a){unsigned mask=0;for(unsigned i=0;i<4;i++)if(a[i].pid>0)mask|=1U<<i;return mask;}
static void sw_packet(struct nova_sw_packet*p,unsigned op,unsigned tag,unsigned index){memset(p,0,sizeof *p);p->magic=NOVA_SW_MAGIC;p->version=1;p->op=op;p->tag=tag;p->index=index;}
static int sw_launch(struct sw_app*a,unsigned index){
 if(a->pid)return a->pid>0?0:EBUSY;int m=posix_openpt(O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC),s=-1;char path[96];if(m<0)return errno;
 if(grantpt(m)||unlockpt(m)||ptsname_r(m,path,sizeof path)||(s=open(path,O_RDWR|O_NOCTTY|O_CLOEXEC))<0){int e=errno;close(m);return e;}
 struct winsize w={.ws_row=24,.ws_col=80};if(ioctl(s,TIOCSWINSZ,&w)){int e=errno;close(m);close(s);return e;}
 pid_t p=fork();if(!p){if(setsid()<0||ioctl(s,TIOCSCTTY,0))_exit(126);for(int i=0;i<3;i++)if(dup2(s,i)<0||fcntl(i,F_SETFD,0)<0)_exit(126);if(close_range(3,~0U,0)||term_install_policy())_exit(126);
 char*args[]={"nova-pkg","run",(char*)nova_sw_ids[index],index==0?"-list":NULL,index==0?"-noheader":NULL,NULL};char*env[]={"HOME=/home/nova","PATH=/bin","LANG=C","TERM=dumb",NULL};if(chdir("/home/nova"))_exit(126);execve("/bin/nova-pkg",args,env);_exit(127);}
 int e=errno;close(s);if(p<0){close(m);return e;}a->pid=p;a->fd=m;a->used=0;return 0;
}
static int sw_loop(void){
 if(getpid()!=1||mount("proc","/proc","proc",MS_NOSUID|MS_NODEV|MS_NOEXEC|MS_RDONLY,NULL)||limit(RLIMIT_AS,32UL*1024*1024)||limit(RLIMIT_NOFILE,64)||limit(RLIMIT_NPROC,16)||limit(RLIMIT_FSIZE,16UL*1024*1024)||limit(RLIMIT_CORE,0)||limit(RLIMIT_CPU,120)||drop())return 1;
 int flags=fcntl(3,F_GETFL);if(flags<0||fcntl(3,F_SETFL,flags|O_NONBLOCK))return 1;
 struct sw_app apps[4];memset(apps,0,sizeof apps);for(unsigned i=0;i<4;i++)apps[i].fd=-1;
 struct nova_sw_packet out;sw_packet(&out,NOVA_SW_READY,0,0);out.code=12004;unsigned outlen=32,last_tag=0,turn=0;int actionfd=-1;pid_t actionpid=0;
 struct nova_sw_state state;memset(&state,0,sizeof state);for(unsigned i=0;i<4;i++)state.installed[i]=4;
 for(;;){
  struct pollfd f[6]={{3,(outlen?POLLOUT:POLLIN),0},{actionfd,actionfd>=0?POLLIN:0,0}};
  for(unsigned i=0;i<4;i++){f[i+2].fd=apps[i].fd;f[i+2].events=(outlen?0:POLLIN)|(apps[i].used?POLLOUT:0);}
  int n=poll(f,6,100);if(n<0&&errno==EINTR)continue;if(n<0)return 1;if(f[0].revents&(POLLERR|POLLHUP|POLLNVAL))return 0;
  if(outlen&&(f[0].revents&POLLOUT)){ssize_t sent=send(3,&out,outlen,MSG_DONTWAIT|MSG_NOSIGNAL);if(sent==(ssize_t)outlen)outlen=0;else if(sent<0&&errno!=EAGAIN&&errno!=EINTR)return 0;}
  if(!outlen&&(f[0].revents&POLLIN)){
   struct nova_sw_packet q;struct iovec io={&q,sizeof q};struct msghdr msg={.msg_iov=&io,.msg_iovlen=1};ssize_t got=recvmsg(3,&msg,MSG_DONTWAIT|MSG_TRUNC);
   if(!got)return 0;if(got<0){if(errno!=EINTR&&errno!=EAGAIN)return 1;}else{
    if(msg.msg_flags||!nova_sw_valid(&q,(unsigned long)got,1)||q.tag<=last_tag)return 1;last_tag=q.tag;unsigned result=0;struct sw_app*a=&apps[q.index];
    if(q.op==NOVA_SW_INPUT){if(a->pid<=0||q.length>sizeof a->input-a->used)result=EBUSY;else{memcpy(a->input+a->used,q.data,q.length);a->used+=q.length;}}
    else if(q.op==NOVA_SW_RESIZE){const unsigned*v=(const void*)q.data;struct winsize w={.ws_row=v[0],.ws_col=v[1]};if(a->fd<0||ioctl(a->fd,TIOCSWINSZ,&w))result=EINVAL;}
    else if(q.op==NOVA_SW_RUN)result=(unsigned)sw_launch(a,q.index);
    else if(q.op==NOVA_SW_CLOSE){if(a->pid>0){int rc=kill(-a->pid,SIGHUP);if(rc&&errno==ESRCH)rc=kill(a->pid,SIGHUP);if(rc&&errno!=ESRCH)result=(unsigned)errno;}}
    else if(actionfd>=0)result=EBUSY;
    else if(q.op==NOVA_SW_REMOVE&&a->pid>0)result=EBUSY;
    else{
     int pipes[2];if(pipe2(pipes,O_CLOEXEC|O_NONBLOCK))result=(unsigned)errno;
     else{actionpid=fork();if(!actionpid){close(pipes[0]);if(dup2(pipes[1],3)<0)_exit(1);if(close_range(4,~0U,0))_exit(1);struct nova_sw_packet response;sw_packet(&response,NOVA_SW_STATE,q.tag,q.index);struct nova_sw_state snapshot;memset(&snapshot,0,sizeof snapshot);snapshot.action=q.op;snapshot.app=q.index;
       if(q.op!=NOVA_SW_LIST)snapshot.result=(unsigned)sw_exec(q.op,q.index);
       for(unsigned i=0;i<4;i++){unsigned value=(unsigned)sw_exec(NOVA_SW_LIST,i);snapshot.installed[i]=value==0?0:value==3?3:value==1?1:4;}
       response.length=sizeof snapshot;memcpy(response.data,&snapshot,sizeof snapshot);ssize_t sent;do sent=write(3,&response,32+response.length);while(sent<0&&errno==EINTR);_exit(sent==(ssize_t)(32+response.length)?0:1);}
      close(pipes[1]);if(actionpid<0){result=(unsigned)errno;close(pipes[0]);}else actionfd=pipes[0];}
    }
    if(q.op==NOVA_SW_RUN||q.op==NOVA_SW_CLOSE||q.op==NOVA_SW_INPUT||q.op==NOVA_SW_RESIZE||result){state.running=sw_mask(apps);state.result=result;state.action=q.op;state.app=q.index;sw_packet(&out,NOVA_SW_STATE,q.tag,q.index);out.length=sizeof state;memcpy(out.data,&state,sizeof state);outlen=32+out.length;}
   }
  }
  if(!outlen&&actionfd>=0&&(f[1].revents&(POLLIN|POLLHUP))){ssize_t got=read(actionfd,&out,sizeof out);if(got>0){if(!nova_sw_valid(&out,(unsigned long)got,0))return 1;memcpy(&state,out.data,sizeof state);state.running=sw_mask(apps);memcpy(out.data,&state,sizeof state);outlen=(unsigned)got;close(actionfd);actionfd=-1;}else if(!got)return 1;}
  for(unsigned j=0;j<4;j++){unsigned i=(turn+j)%4;struct sw_app*a=&apps[i];if(a->fd<0)continue;
   if(a->used&&(f[i+2].revents&POLLOUT)){ssize_t written=write(a->fd,a->input,a->used);if(written>0){a->used-=(unsigned)written;memmove(a->input,a->input+written,a->used);}}
   if(!outlen&&(f[i+2].revents&(POLLIN|POLLHUP|POLLERR))){sw_packet(&out,NOVA_SW_DATA,0,i);ssize_t count=read(a->fd,out.data,sizeof out.data);if(count>0){out.length=(unsigned)count;outlen=32+out.length;turn=(i+1)%4;}else if(!count||(count<0&&errno==EIO)){close(a->fd);a->fd=-1;}}
  }
  int status;pid_t dead;while((dead=waitpid(-1,&status,WNOHANG))>0){if(dead==actionpid)actionpid=0;for(unsigned i=0;i<4;i++)if(apps[i].pid==dead){apps[i].pid=-dead;apps[i].used=0;state.running&=~(1U<<i);apps[i].input[0]=(unsigned char)(WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status));}}
  if(!outlen){for(unsigned i=0;i<4;i++)if(apps[i].pid<0&&apps[i].fd<0){sw_packet(&out,NOVA_SW_EXIT,0,i);out.code=apps[i].input[0];outlen=32;apps[i].pid=0;break;}}
 }
}
int main(int argc,char**argv){
 (void)argv;struct ucred peer;socklen_t len=sizeof peer;int type;socklen_t tl=sizeof type;
 if(argc!=1||getuid()!=0||getsockopt(3,SOL_SOCKET,SO_TYPE,&type,&tl)||type!=SOCK_SEQPACKET||getsockopt(3,SOL_SOCKET,SO_PEERCRED,&peer,&len)||len!=sizeof peer||peer.uid||peer.pid!=getppid())return 126;
 if(close_range(4,~0U,0))return 126;for(int i=0;i<3;i++)close(i);
 if(setup_root()||unshare(CLONE_NEWPID))return 1;pid_t init=fork();if(init<0)return 1;if(!init)_exit(sw_loop());close(3);if(drop())return 1;int status;while(waitpid(init,&status,0)<0)if(errno!=EINTR)return 1;return WIFEXITED(status)?WEXITSTATUS(status):1;
}
