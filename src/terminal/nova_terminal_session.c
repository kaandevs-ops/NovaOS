#define _GNU_SOURCE
/* Trusted private terminal launcher. Root only during namespace construction.
 * No listener, arbitrary command, environment, path or descriptor grant API. */
#include "../runtime/nova_terminal_protocol.h"
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include "nova_terminal_policy.h"
#include "nova_terminal_workspace.h"
#include "nova_desktop_files.h"
static unsigned persistent_workspace;
static int network_fd=-1;static pid_t network_pid=-1;
#ifndef NOVA_NET_BROKER
#define NOVA_NET_BROKER "/System/Core/nova-net-broker"
#endif
#include "../network/nova_net.h"

#ifndef NOVA_SHARED_WORKSPACE_PARENT
#define NOVA_SHARED_WORKSPACE_PARENT "/run/nova/user-session"
#endif
#ifndef NOVA_TERM_ROOT
#define NOVA_TERM_ROOT "/run/nova/terminal-root"
#endif
static int send_packet(unsigned op,unsigned code,const void *data,size_t size,unsigned rows,unsigned cols){
 struct nova_term_packet p={.magic=NOVA_TERM_MAGIC,.version=NOVA_TERM_VERSION,.op=op,.length=size,.rows=rows,.cols=cols,.code=code};
 if(size>sizeof(p.data))return -1;if(size)memcpy(p.data,data,size);
 ssize_t n;do n=send(3,&p,NOVA_TERM_HEADER+size,MSG_NOSIGNAL);while(n<0&&errno==EINTR);
 return n==(ssize_t)(NOVA_TERM_HEADER+size)?0:-1;
}
static int failure(int code){(void)send_packet(NOVA_TERM_ERROR,code,NULL,0,0,0);return 1;}
static int limit(int kind,rlim_t value){struct rlimit r={value,value};return setrlimit(kind,&r);}
static int drop(void){return setgroups(0,NULL)||setgid(NOVA_TERM_UID)||setuid(NOVA_TERM_UID)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||prctl(PR_SET_DUMPABLE,0);}
static int make_dir(const char *p,mode_t m){return mkdir(p,m)==0||errno==EEXIST?0:-1;}
static int bind_ro(const char *source,const char *dest){return mount(source,dest,NULL,MS_BIND,NULL)||mount(NULL,dest,NULL,MS_BIND|MS_REMOUNT|MS_RDONLY|MS_NOSUID|MS_NODEV,NULL);}
static int setup_root(void){
 int shared=-1;int parent=open(NOVA_SHARED_WORKSPACE_PARENT,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(parent>=0){
  if(workspace_directory(parent)){close(parent);return -1;}
  shared=openat(parent,"home",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);close(parent);
  struct stat st;struct statfs fs;
  if(shared<0||fstat(shared,&st)||fstatfs(shared,&fs)||fs.f_type!=0xef53||st.st_uid!=NOVA_TERM_UID||st.st_gid!=NOVA_TERM_UID||(st.st_mode&0777)!=0700){errno=EPERM;return -1;}
 }else if(errno!=ENOENT)return -1;
 int workspace=shared<0?workspace_open():-2;if(workspace==-1)return -1;
 if(unshare(CLONE_NEWNS|CLONE_NEWNET|CLONE_NEWIPC|CLONE_NEWUTS)<0||mount(NULL,"/",NULL,MS_REC|MS_PRIVATE,NULL)<0)return -1;
 /* A directory FD pins its old mount namespace. Reopen under the trusted
  * root-owned parent AFTER unshare, then verify the same inode before bind. */
 if(shared>=0){
  struct stat before,after;if(fstat(shared,&before))return -1;
  int fresh_parent=open(NOVA_SHARED_WORKSPACE_PARENT,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
  if(fresh_parent<0||workspace_directory(fresh_parent))return -1;
  int fresh=openat(fresh_parent,"home",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);close(fresh_parent);
  if(fresh<0||fstat(fresh,&after)||before.st_dev!=after.st_dev||before.st_ino!=after.st_ino||after.st_uid!=NOVA_TERM_UID||after.st_gid!=NOVA_TERM_UID||(after.st_mode&0777)!=0700){errno=EPERM;return -1;}
  close(shared);shared=fresh;
 }
 if(make_dir(NOVA_TERM_ROOT,0700)||mount("tmpfs",NOVA_TERM_ROOT,"tmpfs",MS_NOSUID|MS_NODEV,"mode=0755,size=2m"))return -1;
 if(chdir(NOVA_TERM_ROOT))return -1;
 const char *dirs[]={"bin","etc","lib","lib64","dev","home","tmp","proc","workspace"};
 for(unsigned i=0;i<sizeof(dirs)/sizeof(dirs[0]);i++)if(mkdir(dirs[i],0755))return -1;
 if(bind_ro(NOVA_TERM_BUNDLE "/bin","bin")||bind_ro(NOVA_TERM_BUNDLE "/etc","etc"))return -1;
 /* Optional fixed signed-bundle runtime. Legacy static bundles still work.
  * No client-supplied paths, host root or writable library mounts. */
 int runtime=open(NOVA_TERM_BUNDLE "/Runtime",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(runtime>=0){
  if(workspace_directory(runtime)){close(runtime);return -1;}
  const char *libs[]={"lib","lib64"};
  for(unsigned i=0;i<2;i++){
   int f=openat(runtime,libs[i],O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
   if(f<0||workspace_directory(f)){if(f>=0)close(f);close(runtime);return -1;}close(f);
  }
  const char *objects[]={"lib/libc.so.6","lib/libm.so.6",
#if defined(__x86_64__)
   "lib64/ld-linux-x86-64.so.2"
#else
   "lib/ld-linux-aarch64.so.1"
#endif
  };
  for(unsigned i=0;i<3;i++){
   int f=openat(runtime,objects[i],O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);struct stat st;
   if(f<0||fstat(f,&st)||!S_ISREG(st.st_mode)||st.st_uid||st.st_gid||st.st_nlink!=1||(st.st_mode&07777)!=(i==2?0555:0444)||st.st_size<=0||st.st_size>16*1024*1024){if(f>=0)close(f);close(runtime);errno=EPERM;return -1;}close(f);
  }
  close(runtime);
  if(bind_ro(NOVA_TERM_BUNDLE "/Runtime/lib","lib")||bind_ro(NOVA_TERM_BUNDLE "/Runtime/lib64","lib64"))return -1;
 }else if(errno!=ENOENT)return -1;
 if(mount("tmpfs","home","tmpfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,"mode=0755,size=32m")||mkdir("home/nova",0700)||chown("home/nova",NOVA_TERM_UID,NOVA_TERM_UID))return -1;
 if(shared>=0){
  char source[64];snprintf(source,sizeof source,"/proc/self/fd/%d",shared);
  if(mount(source,"home/nova",NULL,MS_BIND,NULL)||mount(NULL,"home/nova",NULL,MS_BIND|MS_REMOUNT|MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL)){close(shared);return -1;}
  close(shared);persistent_workspace=1;
 }
 if(workspace>=0){
  if(workspace_mount(workspace,"workspace")){close(workspace);return -1;}close(workspace);
  struct stat st;if(stat("workspace",&st)||st.st_uid!=NOVA_TERM_UID||st.st_gid!=NOVA_TERM_UID||(st.st_mode&0777)!=0700){errno=EPERM;return -1;}
  if(mount("workspace","home/nova",NULL,MS_BIND,NULL)||mount(NULL,"home/nova",NULL,MS_BIND|MS_REMOUNT|MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL))return -1;
  /* Only the home bind is reachable after chroot; hide the staging mount. */
  if(umount("workspace")||rmdir("workspace"))return -1;
  persistent_workspace=1;
 }

 if(mount("tmpfs","tmp","tmpfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,"mode=1777,size=8m"))return -1;
 if(mount("tmpfs","dev","tmpfs",MS_NOSUID|MS_NOEXEC,"mode=0755,size=1m")||mkdir("dev/pts",0755))return -1;
 if(mknod("dev/null",S_IFCHR|0666,makedev(1,3))||mknod("dev/zero",S_IFCHR|0666,makedev(1,5)))return -1;
 if(chmod("dev/null",0666)||chmod("dev/zero",0666))return -1;
 if(mount("devpts","dev/pts","devpts",MS_NOSUID|MS_NOEXEC,"newinstance,ptmxmode=0666,mode=0620,gid=12004")||symlink("pts/ptmx","dev/ptmx"))return -1;
 if(chroot(".")||chdir("/"))return -1;
 return 0;
}
/* A private session capability, not a public listener. Broker stays outside
 * the terminal's empty network namespace, drops to a separate UID and chroot. */
static void prepare_network(void){
 if(access(NOVA_NET_BROKER,X_OK))return;
 int pair[2];if(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0,pair))return;
 pid_t pid=fork();if(pid<0){close(pair[0]);close(pair[1]);return;}
 if(!pid){
  close(pair[0]);if(dup2(pair[1],3)<0)_exit(126);if(pair[1]!=3)close(pair[1]);
  for(int i=0;i<3;i++)close(i);
  if(close_range(4,~0U,0))_exit(126);
  char *args[]={NOVA_NET_BROKER,NULL};char *env[]={NULL};execve(args[0],args,env);_exit(126);
 }
 close(pair[1]);if(dup2(pair[0],4)<0){close(pair[0]);return;}if(pair[0]!=4)close(pair[0]);
 struct pollfd p={4,POLLIN,0};struct nova_net_header ready;ssize_t n=-1;
 if(poll(&p,1,2000)>0)n=read(4,&ready,sizeof ready);
 if(n!=sizeof ready||ready.magic!=NOVA_NET_MAGIC||ready.version!=1||ready.tag||ready.length||ready.code){close(4);(void)kill(pid,SIGKILL);(void)waitpid(pid,NULL,0);return;}
 network_fd=4;network_pid=pid;
}
static int init_session(void){
 /* This process is PID 1 in a fresh PID namespace. Exiting it kills EVERY
  * descendant, including daemonized jobs and those that ignore SIGHUP. */
 if(getpid()!=1||mount("proc","/proc","proc",MS_NOSUID|MS_NODEV|MS_NOEXEC|MS_RDONLY,NULL))return failure(errno);
 int m=posix_openpt(O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC),s=-1;
 char slave[96];if(m<0||grantpt(m)||unlockpt(m)||ptsname_r(m,slave,sizeof slave))return failure(errno);
 s=open(slave,O_RDWR|O_NOCTTY|O_CLOEXEC);if(s<0)return failure(errno);
 struct winsize ws={.ws_row=24,.ws_col=80};if(ioctl(s,TIOCSWINSZ,&ws))return failure(errno);
 if(limit(RLIMIT_AS,32UL*1024*1024)||limit(RLIMIT_NOFILE,64)||limit(RLIMIT_NPROC,16)||limit(RLIMIT_FSIZE,16UL*1024*1024)||limit(RLIMIT_CORE,0)||limit(RLIMIT_CPU,120))return failure(errno);
 int report[2];if(pipe2(report,O_CLOEXEC))return failure(errno);
 pid_t shell=fork();if(shell<0)return failure(errno);
 if(!shell){
  close(report[0]);close(m);close(3);
  if(setsid()<0||ioctl(s,TIOCSCTTY,0))goto child_fail;
  for(int i=0;i<3;i++)if(dup2(s,i)<0||fcntl(i,F_SETFD,0)<0)goto child_fail;
  if(s>2)close(s);
  if(network_fd>=0&&fcntl(network_fd,F_SETFD,0)<0)goto child_fail;
  if(setpriority(PRIO_PROCESS,0,5)||drop()||chdir("/home/nova")||term_install_policy())goto child_fail;
  /* Fixed signed-bundle discovery banner; no automatic user commands. */
  static const char welcome[]="NovaOS terminal: real BusyBox ash\r\n"
    "Apps: nova-guide | nova-calendar | nova-clock\r\n"
    "Help: busybox --list | COMMAND --help\r\n";
  size_t banner_done=0;
  while(banner_done<sizeof(welcome)-1){
   ssize_t count=write(STDOUT_FILENO,welcome+banner_done,sizeof(welcome)-1-banner_done);
   if(count<0&&errno==EINTR)continue;
   if(count<=0){if(!count)errno=EIO;goto child_fail;}
   banner_done+=(size_t)count;
  }
  char *argv[]={"sh","-i",NULL};
  char *env[]={"HOME=/home/nova","PATH=/bin","TERM=dumb","LANG=C","USER=nova","LOGNAME=nova","PS1=nova$ ",NULL};
  execve("/bin/busybox",argv,env);
child_fail: {int e=errno;ssize_t sent;do sent=write(report[1],&e,sizeof e);while(sent<0&&errno==EINTR);_exit(126);}
 }
 close(report[1]);close(s);if(network_fd>=0)close(network_fd);
 int e=0;ssize_t rn;do rn=read(report[0],&e,sizeof e);while(rn<0&&errno==EINTR);close(report[0]);
 if(rn!=0)return failure(e?e:EIO);
 if(drop())return failure(errno);
 if(send_packet(NOVA_TERM_READY,NOVA_TERM_UID|(persistent_workspace?NOVA_TERM_PERSISTENT:0),NULL,0,24,80))return 1;
 int socket_flags=fcntl(3,F_GETFL);if(socket_flags<0||fcntl(3,F_SETFL,socket_flags|O_NONBLOCK))return 1;
 unsigned char input[4096];size_t used=0;
 struct nova_term_packet output;size_t out_len=0;
 int status=0,exited=0;
 struct nova_desktop_snapshot desktop_reply;int desktop_pending=0;
 struct nova_desktop_file_reply file_reply;struct nova_desktop_trash_snapshot trash_reply;unsigned file_pending=0;
 for(;;){
  struct pollfd fds[2]={{3,(used<=sizeof input-NOVA_TERM_BYTES?POLLIN:0)|(out_len?POLLOUT:0),0},{m,(!out_len?POLLIN:0)|(used?POLLOUT:0),0}};
  int r=poll(fds,2,100);if(r<0&&errno==EINTR)continue;if(r<0)return 1;
  if(fds[0].revents&(POLLHUP|POLLERR|POLLNVAL))return 0;
  if(fds[0].revents&POLLIN){
   struct nova_term_packet req;ssize_t n=recv(3,&req,sizeof req,MSG_TRUNC|MSG_DONTWAIT);
   if(n==0)return 0;
   if(n>0){
    if(!nova_term_valid(&req,(unsigned long)n,1))return failure(EPROTO);
    if(req.op==NOVA_TERM_DESKTOP){
     if(desktop_pending)return failure(EBUSY);
     int home=open("/home/nova",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
     if(home<0){memset(&desktop_reply,0,sizeof desktop_reply);desktop_reply.status=errno;}
     else{(void)nova_desktop_action(home,req.rows,&desktop_reply);close(home);}desktop_pending=1;
    }
    if(req.op==NOVA_TERM_DESKTOP_FILE){
     if(file_pending)return failure(EBUSY);
     struct nova_desktop_file_request request;memcpy(&request,req.data,sizeof request);
     int home=open("/home/nova",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
     if(request.action==NOVA_DESKTOP_TRASH_LIST){
      if(home<0){memset(&trash_reply,0,sizeof trash_reply);trash_reply.status=errno;}
      else (void)nova_desktop_trash_page(home,request.offset,&trash_reply);
      file_pending=NOVA_TERM_DESKTOP_TRASH_STATE;
     }else{
      if(home<0){memset(&file_reply,0,sizeof file_reply);file_reply.status=errno;file_reply.action=request.action;}
      else if(request.action==NOVA_DESKTOP_PREVIEW)(void)nova_desktop_preview(home,&request,&file_reply);
      else (void)nova_desktop_trash_change(home,&request,&file_reply);
      file_pending=NOVA_TERM_DESKTOP_FILE_STATE;
     }if(home>=0)close(home);
    }
    if(req.op==NOVA_TERM_CLOSE)return 0;
    if(req.op==NOVA_TERM_RESIZE){struct winsize w={.ws_row=req.rows,.ws_col=req.cols};if(ioctl(m,TIOCSWINSZ,&w))return failure(errno);}
    if(req.op==NOVA_TERM_INPUT){if(req.length>sizeof(input)-used)return failure(ENOBUFS);memcpy(input+used,req.data,req.length);used+=req.length;}
   }else if(errno==ECONNRESET)return 0;else if(errno!=EAGAIN&&errno!=EINTR)return 1;
  }
  if(used&&(fds[1].revents&POLLOUT)){ssize_t n=write(m,input,used);if(n>0){used-=(size_t)n;memmove(input,input+n,used);}else if(n<0&&errno!=EAGAIN&&errno!=EINTR)return 1;}
  if(out_len&&(fds[0].revents&POLLOUT)){ssize_t n=send(3,&output,out_len,MSG_NOSIGNAL|MSG_DONTWAIT);if(n==(ssize_t)out_len)out_len=0;else if(n<0&&(errno==ECONNRESET||errno==EPIPE))return 0;else if(n<0&&errno!=EAGAIN&&errno!=EINTR)return 1;}
  /* Replies have a bounded turn before PTY output, so a busy program cannot
   * starve a desktop open/restore response. PTY bytes remain queued in kernel. */
  if(!out_len&&file_pending){memset(&output,0,sizeof output);output.magic=NOVA_TERM_MAGIC;output.version=NOVA_TERM_VERSION;output.op=file_pending;output.length=file_pending==NOVA_TERM_DESKTOP_TRASH_STATE?sizeof trash_reply:sizeof file_reply;memcpy(output.data,file_pending==NOVA_TERM_DESKTOP_TRASH_STATE?(void*)&trash_reply:(void*)&file_reply,output.length);out_len=NOVA_TERM_HEADER+output.length;file_pending=0;}
  if(desktop_pending&&!out_len){memset(&output,0,sizeof output);output.magic=NOVA_TERM_MAGIC;output.version=NOVA_TERM_VERSION;output.op=NOVA_TERM_DESKTOP_STATE;output.length=sizeof desktop_reply;memcpy(output.data,&desktop_reply,sizeof desktop_reply);out_len=NOVA_TERM_HEADER+sizeof desktop_reply;desktop_pending=0;}
  if(!out_len&&(fds[1].revents&(POLLIN|POLLHUP))){
   ssize_t n=read(m,output.data,sizeof output.data);
   if(n>0){output.magic=NOVA_TERM_MAGIC;output.version=NOVA_TERM_VERSION;output.op=NOVA_TERM_DATA;output.length=n;output.rows=output.cols=output.code=output.reserved=0;out_len=NOVA_TERM_HEADER+n;}
   else if(n==0||(n<0&&errno==EIO)){
    if(!exited){while(waitpid(shell,&status,0)<0&&errno==EINTR){}exited=1;}
    (void)send_packet(NOVA_TERM_EXIT,WIFEXITED(status)?(unsigned)WEXITSTATUS(status):128U+(unsigned)WTERMSIG(status),NULL,0,0,0);return 0;
   }else if(errno!=EAGAIN&&errno!=EINTR)return 1;
  }

  /* Reap adopted background jobs, but drain remaining PTY output before EXIT. */
  pid_t reaped;int st;
  while((reaped=waitpid(-1,&st,WNOHANG))>0)if(reaped==shell){status=st;exited=1;}
 }
}
int main(int argc,char **argv){
 (void)argv;
 struct ucred cred; socklen_t size=sizeof cred;int type=0;socklen_t ts=sizeof type;
 if(argc!=1||getuid()!=0||getsockopt(3,SOL_SOCKET,SO_TYPE,&type,&ts)||type!=SOCK_SEQPACKET||getsockopt(3,SOL_SOCKET,SO_PEERCRED,&cred,&size)||size!=sizeof cred||cred.uid!=0||cred.pid!=getppid())return 126;
 /* Only the private channel survives. No DRM, novad, directory or log FD. */
 if(close_range(4,~0U,0)<0)return failure(errno);
 for(int i=0;i<3;i++)close(i);
 prepare_network();
 if(setup_root())return failure(errno);
 /* Session init owns PID 1 of the terminal namespace. */
 if(unshare(CLONE_NEWPID)<0)return failure(errno);
 pid_t init=fork();if(init<0)return failure(errno);
 if(!init)_exit(init_session());
 close(3);if(network_fd>=0)close(network_fd);if(drop())return 1;
 int status;while(waitpid(init,&status,0)<0){if(errno!=EINTR)return 1;}
 if(network_pid>0)(void)waitpid(network_pid,NULL,WNOHANG);
 return WIFEXITED(status)?WEXITSTATUS(status):1;
}
