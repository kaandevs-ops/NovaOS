#define _GNU_SOURCE
/* Fixed trusted bootstrap, signed System only. Same PID execs bridge so novad's
 * PID/generation authority is not delegated to an unregistered grandchild.
 * Firefox/Xvfb live below a private PID1; no host home, proc or device tree. */
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
/* Cold XKB/font startup may exceed 15 seconds on a busy VM host. Keep a
 * bounded monotonic deadline; signals must not turn readiness into failure. */
static int wait_readable(int fd,int timeout_ms){
 struct timespec now;if(clock_gettime(CLOCK_MONOTONIC,&now))return 0;
 long long deadline=(long long)now.tv_sec*1000+now.tv_nsec/1000000+timeout_ms;
 for(;;){if(clock_gettime(CLOCK_MONOTONIC,&now))return 0;
  long long left=deadline-((long long)now.tv_sec*1000+now.tv_nsec/1000000);if(left<=0)return 0;
  struct pollfd p={.fd=fd,.events=POLLIN};int n=poll(&p,1,(int)left);
  if(n<0&&errno==EINTR)continue;return n>0&&(p.revents&POLLIN);
 }
}
/* A pipe write may be split at any byte boundary. Never accept a partial
 * number or an empty newline as X readiness; retain one monotonic deadline. */
static int read_display_number(int fd,int timeout_ms,long*out){
 struct timespec ts;if(!out||timeout_ms<=0||clock_gettime(CLOCK_MONOTONIC,&ts))return 0;
 long long deadline=(long long)ts.tv_sec*1000+ts.tv_nsec/1000000+timeout_ms;char digits[3];unsigned used=0;
 for(;;){
  if(clock_gettime(CLOCK_MONOTONIC,&ts))return 0;
  long long left=deadline-((long long)ts.tv_sec*1000+ts.tv_nsec/1000000);
  if(left<=0||!wait_readable(fd,(int)left))return 0;
  char c;ssize_t n=read(fd,&c,1);if(n<0&&errno==EINTR)continue;if(n!=1)return 0;
  if(c=='\n'){if(!used)return 0;digits[used]=0;char*end;long value=strtol(digits,&end,10);if(*end||value<0||value>63)return 0;*out=value;return 1;}
  if(c<'0'||c>'9'||used>=2)return 0;digits[used++]=c;
 }
}
#ifndef NOVA_BROWSER_RUNTIME
#define NOVA_BROWSER_RUNTIME "/System/Browser"
#endif
#ifndef NOVA_BROWSER_ROOT
#define NOVA_BROWSER_ROOT "/run/nova/browser-root"
#endif
#ifndef NOVA_BROWSER_IPC
#define NOVA_BROWSER_IPC "/run/nova/app"
#endif
#define UID 4
static int drop(void){return setgroups(0,NULL)||setresgid(UID,UID,UID)||setresuid(UID,UID,UID)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||prctl(PR_SET_DUMPABLE,0);}
static int directory(const char*p){int fd=open(p,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);struct stat s;if(fd<0)return -1;int bad=fstat(fd,&s)||s.st_uid||s.st_gid||(s.st_mode&0022);close(fd);if(bad){errno=EPERM;return -1;}return 0;}
static int md(const char*p,mode_t mode){if(mkdir(p,mode)&&errno!=EEXIST)return -1;return directory(p);}
static int ro(const char*s,const char*d){return mount(s,d,NULL,MS_BIND,NULL)||mount(NULL,d,NULL,MS_BIND|MS_REMOUNT|MS_RDONLY|MS_NOSUID|MS_NODEV,NULL);}
static int limits(void){struct rlimit f={1024,1024},n={512,512},c={0,0};return setrlimit(RLIMIT_NOFILE,&f)||setrlimit(RLIMIT_NPROC,&n)||setrlimit(RLIMIT_CORE,&c);}
static int setup(void){
 struct statvfs fs;if(directory(NOVA_BROWSER_RUNTIME)||statvfs(NOVA_BROWSER_RUNTIME,&fs)||!(fs.f_flag&ST_RDONLY)){errno=EPERM;return -1;}
 if(directory(NOVA_BROWSER_IPC))return -1;
 if(unshare(CLONE_NEWNS|CLONE_NEWIPC|CLONE_NEWUTS)||mount(NULL,"/",NULL,MS_REC|MS_PRIVATE,NULL))return -1;
 if(md(NOVA_BROWSER_ROOT,0700)||mount("tmpfs",NOVA_BROWSER_ROOT,"tmpfs",MS_NOSUID|MS_NODEV,"mode=0755,size=2m")||chdir(NOVA_BROWSER_ROOT))return -1;
 const char*dirs[]={"bin","etc","lib","lib64","opt","usr","dev","home","tmp","proc","run"};
 for(unsigned i=0;i<sizeof(dirs)/sizeof(dirs[0]);i++)if(mkdir(dirs[i],0755)||chmod(dirs[i],0755))return -1;
 const char*fixed[]={"bin","etc","lib","lib64","opt","usr"};char source[256];
 for(unsigned i=0;i<sizeof(fixed)/sizeof(fixed[0]);i++){if(snprintf(source,sizeof source,"%s/%s",NOVA_BROWSER_RUNTIME,fixed[i])>=(int)sizeof source||directory(source)||ro(source,fixed[i]))return -1;}
 if(mount("tmpfs","tmp","tmpfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,"mode=1777,size=64m")||mount("tmpfs","home","tmpfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,"mode=0755,size=256m")||mount("tmpfs","dev","tmpfs",MS_NOSUID|MS_NOEXEC,"mode=0755,size=1m"))return -1;
 if(mkdir("home/nova",0700)||chown("home/nova",UID,UID)||mkdir("home/nova/profile",0700)||chown("home/nova/profile",UID,UID)||mkdir("home/nova/runtime",0700)||chown("home/nova/runtime",UID,UID))return -1;
 const char*names[]={"null","zero","random","urandom"};unsigned minors[]={3,5,8,9};
 for(unsigned i=0;i<4;i++){char path[48];snprintf(path,sizeof path,"dev/%s",names[i]);if(mknod(path,S_IFCHR|0666,makedev(1,minors[i]))||chmod(path,0666))return -1;}
 if(mkdir("dev/shm",01777)||mount("tmpfs","dev/shm","tmpfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,"mode=1777,size=64m")||mkdir("tmp/.X11-unix",01777)||chmod("tmp/.X11-unix",01777))return -1;
 if(mkdir("run/nova",0755)||chmod("run/nova",0755)||mkdir("run/nova/app",0755)||ro(NOVA_BROWSER_IPC,"run/nova/app"))return -1;
 /* Wildcard-family cookie for this one private X server; no -ac, no TCP. */
 unsigned char cookie[16];if(getrandom(cookie,sizeof cookie,0)!=(ssize_t)sizeof cookie)return -1;
 const unsigned char prefix[]={255,255,0,0,0,0,0,18};const char auth[]="MIT-MAGIC-COOKIE-1";unsigned char length[]={0,16};
 int fd=open("tmp/Xauthority",O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);if(fd<0)return -1;
 int bad=write(fd,prefix,sizeof prefix)!=(ssize_t)sizeof prefix||write(fd,auth,18)!=18||write(fd,length,2)!=2||write(fd,cookie,16)!=16||fchown(fd,UID,UID);close(fd);if(bad)return -1;
 /* Release only this cloned namespace's old shared-home/Data mounts.
  * Otherwise the hidden user-home loop mount can pin its backing Data file.
  * mount-move + chroot makes this the namespace root even when NovaOS boots
  * from the special initial rootfs, where pivot_root is not supported. */
 if(umount2("/run/nova/user-session/home",MNT_DETACH)&&errno!=ENOENT&&errno!=EINVAL)return -1;
 if(umount2("/Data",MNT_DETACH)&&errno!=ENOENT&&errno!=EINVAL)return -1;
 if(mount(".","/",NULL,MS_MOVE,NULL)||chroot(".")||chdir("/"))return -1;
 return limits();
}
static char*env[]={"HOME=/home/nova","PATH=/usr/bin:/bin","XAUTHORITY=/tmp/Xauthority","DISPLAY=unix/:0","XDG_RUNTIME_DIR=/home/nova/runtime","MOZ_ENABLE_WAYLAND=0","LANG=C.UTF-8",NULL};
static volatile sig_atomic_t stopping;
static void stop(int sig){(void)sig;stopping=1;}
static int child_exec(char*const args[],int ready){
 pid_t p=fork();if(p<0)return -1;if(!p){
  if(ready>=0){if(dup2(ready,9)<0||fcntl(9,F_SETFD,0)<0)_exit(126);}if(close_range(3,8,0)||close_range(10,~0U,0))_exit(126);
  if(ready<0)close(9);execve(args[0],args,env);_exit(127);
 }return p;
}
static int runtime_init(int control){
 /* CLONE_NEWPID took effect at this fork. Further mount isolation keeps the
  * namespace proc mount out of the supervising bridge's view. */
 if(getpid()!=1||unshare(CLONE_NEWNS)||mount(NULL,"/",NULL,MS_REC|MS_PRIVATE,NULL)||mount("proc","/proc","proc",MS_NOSUID|MS_NODEV|MS_NOEXEC,"hidepid=2")||mount("/proc/sys","/proc/sys",NULL,MS_BIND,NULL)||mount(NULL,"/proc/sys",NULL,MS_BIND|MS_REMOUNT|MS_RDONLY|MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL))return 1;
 signal(SIGTERM,stop);signal(SIGINT,stop);if(drop())return 1;
#ifndef NOVA_BROWSER_LAUNCHER_HOST_TEST
 /* Required in the real guest: never silently ship with the Firefox
  * namespace sandbox blocked. Host AppArmor may reject the diagnostic build. */
 uid_t outside_uid=getuid();gid_t outside_gid=getgid();
 pid_t probe=fork();if(probe<0)return 1;if(!probe){
  if(unshare(CLONE_NEWUSER)){dprintf(2,"[nova-browser] probe_clone_errno=%d\n",errno);_exit(126);}
  /* No exec occurs in this probe. Restore normal user dumpability only in
   * this ephemeral child so procfs self-map ownership remains UID4, exactly
   * as after an ordinary Firefox exec. Parent/core policy is unchanged. */
  if(prctl(PR_SET_DUMPABLE,1,0,0,0))_exit(126);
  const char*paths[]={"/proc/self/uid_map","/proc/self/setgroups","/proc/self/gid_map"};
  char uid[64],gid[64];snprintf(uid,sizeof uid,"0 %lu 1\n",(unsigned long)outside_uid);snprintf(gid,sizeof gid,"0 %lu 1\n",(unsigned long)outside_gid);const char*values[]={uid,"deny\n",gid};
  for(unsigned i=0;i<3;i++){int f=open(paths[i],O_WRONLY|O_CLOEXEC);if(f<0){dprintf(2,"[nova-browser] probe_open=%s errno=%d\n",paths[i],errno);_exit(126);}size_t length=strlen(values[i]);ssize_t n=write(f,values[i],length);close(f);if(n!=(ssize_t)length){dprintf(2,"[nova-browser] probe_write=%s errno=%d\n",paths[i],errno);_exit(126);}}
  int denied=open("/proc/sys/kernel/hostname",O_WRONLY|O_CLOEXEC);if(denied>=0){close(denied);_exit(126);}
  _exit(getuid()==0&&getgid()==0?0:126);
 }
 int probe_status;if(waitpid(probe,&probe_status,0)!=probe||!WIFEXITED(probe_status)||WEXITSTATUS(probe_status)){dprintf(2,"[nova-browser] user_namespace_probe=FAIL\n");return 1;}
 dprintf(2,"[nova-browser] user_namespace_probe=PASS mapping=uid-gid sysctl=readonly\n");
#endif
 int pipefd[2];if(pipe2(pipefd,O_CLOEXEC))return 1;
 char*xargs[]={"/usr/bin/Xvfb",":0","-displayfd","9","-screen","0","1024x650x24","-nolisten","tcp","-nolisten","local","-auth","/tmp/Xauthority","-noreset",NULL};
 pid_t x=child_exec(xargs,pipefd[1]);close(pipefd[1]);if(x<0){close(pipefd[0]);return 1;}
 dprintf(2,"[nova-browser] X readiness readfd=%d writerfd=%d pid=%ld\n",pipefd[0],pipefd[1],(long)x);
 long number=-1;int okay=read_display_number(pipefd[0],60000,&number);close(pipefd[0]);
 dprintf(2,"[nova-browser] X readiness complete=%d display=%ld\n",okay,number);if(!okay){kill(x,SIGTERM);return 1;}
 char display[40];snprintf(display,sizeof display,"DISPLAY=unix/:%ld",number);env[3]=display;
 char*ffargs[]={"/opt/firefox/firefox","--no-remote","--profile","/home/nova/profile","--new-window","file:///usr/share/nova/start.html","--width","1024","--height","650",NULL};
 pid_t firefox=child_exec(ffargs,-1);if(firefox<0){kill(x,SIGTERM);return 1;}
 unsigned ready_message[2]={0x4e425244U,(unsigned)number};if(send(control,ready_message,sizeof ready_message,MSG_NOSIGNAL)!=sizeof ready_message){kill(firefox,SIGTERM);kill(x,SIGTERM);return 1;}
 int browser_done=0,x_done=0;unsigned grace=0,term_sent=0;
 while(!browser_done||!x_done){
  struct pollfd p={.fd=control,.events=POLLIN};(void)poll(&p,1,100);
  if(p.revents&(POLLIN|POLLHUP|POLLERR))stopping=1;
  int status;pid_t done;while((done=waitpid(-1,&status,WNOHANG))>0){if(done==firefox){browser_done=1;dprintf(2,"[nova-browser] firefox_wait_status=%d\n",status);}if(done==x)x_done=1;}
  if(browser_done||x_done)stopping=1;
  if(stopping){if(control>=0){close(control);control=-1;}if(!browser_done&&grace>=30&&!term_sent){kill(firefox,SIGTERM);term_sent=1;}if(!x_done&&((browser_done&&grace>=10)||grace>=90))kill(x,SIGTERM);if(++grace>=100)break;}
 }
 /* Exiting private PID1 tears down only this browser namespace on crash. */
 return browser_done?0:1;
}
int main(int argc,char**argv){
 (void)argv;if(argc!=1||geteuid()!=0)return 126;
#ifndef NOVA_BROWSER_LAUNCHER_HOST_TEST
 if(getppid()!=1)return 126;
#endif
 umask(077);if(close_range(3,~0U,0)||setup()){perror("browser private runtime");return 126;}
 int control[2];if(socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0,control)||unshare(CLONE_NEWPID))return 126;
 pid_t init=fork();if(init<0)return 126;if(!init){close(control[0]);int rc=runtime_init(control[1]);_exit(rc);}
 close(control[1]);unsigned ready[2];
 if(!wait_readable(control[0],75000)||recv(control[0],ready,sizeof ready,0)!=sizeof ready||ready[0]!=0x4e425244U||ready[1]>63){close(control[0]);(void)waitpid(init,NULL,0);return 126;}
 if(dup2(control[0],10)<0)return 126;if(control[0]!=10)close(control[0]);
 if(drop())return 126;char pid[64];snprintf(pid,sizeof pid,"NOVA_BROWSER_INIT_PID=%ld",(long)init);
 char display[48];snprintf(display,sizeof display,"NOVA_BROWSER_DISPLAY=%u",ready[1]);
 char*e[]={"HOME=/home/nova","PATH=/usr/bin:/bin","XAUTHORITY=/tmp/Xauthority","DISPLAY=unix/:0","XDG_RUNTIME_DIR=/home/nova/runtime","LANG=C.UTF-8",pid,display,NULL};
 char*args[]={"/usr/bin/nova-browser-bridge",NULL};execve(args[0],args,e);close(10);(void)waitpid(init,NULL,0);return 127;
}
