#define _GNU_SOURCE
#include "nova_repository.h"
#include "../runtime/nova_store_protocol.h"
#include "../runtime/nova_local_catalog.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <grp.h>
#include <sched.h>
#include <signal.h>
#include <sys/mount.h>
#include <linux/mount.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <poll.h>
#include <openssl/evp.h>
#include "nova_store_policy.h"
#include "../terminal/nova_terminal_policy.h"
#ifndef NOVA_STORE_ROOT
#define NOVA_STORE_ROOT "/System/Software"
#endif
#ifndef NOVA_STORE_HOME
#define NOVA_STORE_HOME "/run/nova/user-session/home"
#endif
static int root_file(const char*path,unsigned char*b,size_t cap){int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);if(fd<0)return -1;struct stat s;if(fstat(fd,&s)||!S_ISREG(s.st_mode)||s.st_uid||s.st_gid||s.st_nlink!=1||(s.st_mode&0022)||s.st_size<1||(uint64_t)s.st_size>cap){close(fd);return -1;}size_t used=0;while(used<(size_t)s.st_size){ssize_t n=read(fd,b+used,(size_t)s.st_size-used);if(n<0&&errno==EINTR)continue;if(n<=0){close(fd);return -1;}used+=(size_t)n;}close(fd);return (int)used;}
/* Mount descriptors are resolved in the original namespace before chroot. */
static unsigned setup_stage;
static int isolate(void){setup_stage=1;
 struct stat s;int root=open(NOVA_STORE_ROOT,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC),home=open(NOVA_STORE_HOME,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(root<0||home<0||fstat(root,&s)||s.st_uid||s.st_gid||(s.st_mode&0022)||fstat(home,&s)||s.st_uid!=12004||(s.st_mode&0777)!=0700)return -1;
 setup_stage=2;int made=mkdirat(home,".nova-apps",0700);if(made&&errno!=EEXIST)return -1;int store=openat(home,".nova-apps",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);close(home);if(store>=0&&!made&&fchown(store,12004,12004))return -1;if(store<0||fstat(store,&s)||s.st_uid!=12004||(s.st_mode&0777)!=0700)return -1;
 int tree=(int)syscall(SYS_open_tree,store,"",OPEN_TREE_CLONE|OPEN_TREE_CLOEXEC|AT_EMPTY_PATH);if(tree<0)return -1;
 setup_stage=3;if(unshare(CLONE_NEWNS|CLONE_NEWIPC|CLONE_NEWUTS)||mount(NULL,"/",NULL,MS_REC|MS_PRIVATE,NULL))return -1;
 char source[64],target[4096];snprintf(source,sizeof source,"/proc/self/fd/%d",root);
 setup_stage=41;if(mount(NOVA_STORE_ROOT,NOVA_STORE_ROOT,NULL,MS_BIND,NULL))return -1;setup_stage=42;if(mount(NULL,NOVA_STORE_ROOT,NULL,MS_BIND|MS_REMOUNT|MS_RDONLY|MS_NOSUID|MS_NODEV,NULL))return -1;
 snprintf(source,sizeof source,"/proc/self/fd/%d",store);snprintf(target,sizeof target,"%s/home/nova/.nova-apps",NOVA_STORE_ROOT);
 setup_stage=5;if(syscall(SYS_move_mount,tree,"",AT_FDCWD,target,MOVE_MOUNT_F_EMPTY_PATH)||mount(NULL,target,NULL,MS_BIND|MS_REMOUNT|MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL))return -1;close(tree);close(store);
 setup_stage=6;snprintf(target,sizeof target,"%s/tmp",NOVA_STORE_ROOT);if(mount("tmpfs",target,"tmpfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,"mode=0700,uid=12004,gid=12004,size=16m"))return -1;
 setup_stage=7;snprintf(target,sizeof target,"%s/dev",NOVA_STORE_ROOT);if(mount("tmpfs",target,"tmpfs",MS_NOSUID,"mode=0755,size=64k"))return -1;
 setup_stage=8;snprintf(target,sizeof target,"%s/dev/null",NOVA_STORE_ROOT);if(mknod(target,S_IFCHR|0666,makedev(1,3))||chmod(target,0666))return -1;
 setup_stage=9;close(root);root=open(NOVA_STORE_ROOT,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(root<0||fchdir(root)||chroot(".")||chdir("/"))return -1;close(root);return 0;
}
static int emit(unsigned op,unsigned tag,unsigned app,unsigned bytes,unsigned total,unsigned code){struct nova_store_packet p={NS_MAGIC,1,op,tag,app,bytes,total,code};ssize_t n=send(3,&p,sizeof p,MSG_DONTWAIT|MSG_NOSIGNAL);return n==sizeof p?0:-1;}
static int install_package(int fd,unsigned app){pid_t p=fork();if(p<0)return EAGAIN;if(!p){if(dup2(fd,4)<0||fcntl(4,F_SETFD,0)<0)_exit(126);int d=open("/dev/null",O_RDWR);if(d<0)_exit(126);for(int i=0;i<3;i++)if(dup2(d,i)<0||fcntl(i,F_SETFD,0)<0)_exit(126);if(dup2(4,3)<0||fcntl(3,F_SETFD,0)<0||close_range(4,~0U,0)||term_install_policy())_exit(126);char*args[]={"nova-pkg","install-fd",(char*)nova_catalog[app].id,"3",NULL};char*env[]={"HOME=/home/nova","PATH=/bin","LANG=C",NULL};execve("/bin/nova-pkg",args,env);_exit(127);}int st;if(waitpid(p,&st,0)!=p)return EIO;return WIFEXITED(st)&&!WEXITSTATUS(st)?0:EIO;}
static int committed(unsigned app){char path[128];snprintf(path,sizeof path,"/home/nova/.nova-apps/%s",nova_catalog[app].id);int fd=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);if(fd<0)return 0;struct stat st;if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_uid!=12004||st.st_nlink!=1||st.st_size<1||st.st_size>NR_MAX_PAYLOAD){close(fd);return 0;}EVP_MD_CTX*m=EVP_MD_CTX_new();unsigned char b[16384],digest[32];unsigned n=0;int ok=m&&EVP_DigestInit_ex(m,EVP_sha256(),NULL)==1;ssize_t got=0;while(ok&&(got=read(fd,b,sizeof b))>0)ok=EVP_DigestUpdate(m,b,(size_t)got)==1;if(ok&&(got<0||EVP_DigestFinal_ex(m,digest,&n)!=1||n!=32))ok=0;EVP_MD_CTX_free(m);close(fd);if(!ok)return 0;char hex[65];for(unsigned i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",digest[i]);return !strcmp(hex,nova_catalog[app].sha256);}
static long long seconds_now(void){struct timespec t;return clock_gettime(CLOCK_MONOTONIC,&t)?0:t.tv_sec;}
static void end_job(pid_t child){if(child>0){if(kill(-child,SIGTERM)&&errno==ESRCH)(void)kill(child,SIGTERM);while(waitpid(child,NULL,0)<0&&errno==EINTR){}}}
int main(int argc,char**argv){(void)argv;struct ucred peer;socklen_t sz=sizeof peer;int type;socklen_t tz=sizeof type;
 if(argc!=1||getuid()!=0||getsockopt(3,SOL_SOCKET,SO_TYPE,&type,&tz)||type!=SOCK_SEQPACKET||getsockopt(3,SOL_SOCKET,SO_PEERCRED,&peer,&sz)||peer.uid||peer.pid!=getppid()||close_range(4,~0U,0))return 126;
 for(int i=0;i<3;i++)close(i);if(isolate()){(void)emit(NS_DONE,1,0,setup_stage,setup_stage,(unsigned)(errno?errno:EPERM));return 1;}
 unsigned char key[32],wire[NR_MAX_INDEX+64];int keylen=root_file("/etc/repo.pub",key,sizeof key),length=root_file("/etc/repository.nri",wire,sizeof wire);struct nr_index index;unsigned available=0,map[4]={0};
#ifdef __x86_64__
 const char*arch="x86_64";
#else
 const char*arch="arm64";
#endif
 if(keylen==32&&length>64&&!nr_verify(wire,(size_t)length,key,arch,1,(uint64_t)time(NULL),&index))for(unsigned a=0;a<4;a++)for(unsigned i=0;i<index.count;i++)if(!strcmp(index.entries[i].id,nova_catalog[a].id)&&!strcmp(index.entries[i].version,nova_catalog[a].version)&&!strcmp(index.entries[i].sha256,nova_catalog[a].sha256)){available|=1U<<a;map[a]=i;}
 struct rlimit mem={256UL*1024*1024,256UL*1024*1024},files={48,48},cpu={120,120},core={0,0},count={32,32},size={NR_MAX_PAYLOAD,NR_MAX_PAYLOAD};
 if(setrlimit(RLIMIT_AS,&mem)||setrlimit(RLIMIT_NOFILE,&files)||setrlimit(RLIMIT_CPU,&cpu)||setrlimit(RLIMIT_CORE,&core)||setrlimit(RLIMIT_FSIZE,&size)||setrlimit(RLIMIT_NPROC,&count)||setgroups(0,NULL)||setgid(12004)||setuid(12004)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||prctl(PR_SET_DUMPABLE,0)||prctl(PR_SET_PDEATHSIG,SIGTERM)||store_policy())return 1;
 signal(SIGPIPE,SIG_IGN);int flags=fcntl(3,F_GETFL);if(flags<0||fcntl(3,F_SETFL,flags|O_NONBLOCK)||emit(NS_READY,0,0,0,0,available))return 1;
 pid_t job=0;long long started=0;int payload=-1;unsigned app=0,tag=0,last=0,total=0,previous=~0U;
 for(;;){struct pollfd f={3,POLLIN,0};int rc=poll(&f,1,100);if(rc<0&&errno==EINTR)continue;if(rc<0||f.revents&(POLLHUP|POLLERR|POLLNVAL)){end_job(job);return 0;}
  if(f.revents&POLLIN){struct nova_store_packet q;struct iovec io={&q,sizeof q};struct msghdr m={.msg_iov=&io,.msg_iovlen=1};ssize_t n=recvmsg(3,&m,MSG_TRUNC|MSG_DONTWAIT);if(n<0&&(errno==EINTR||errno==EAGAIN))continue;if(n<=0||m.msg_flags||!nova_store_valid(&q,(unsigned long)n,1)||q.tag<=last){end_job(job);return 1;}last=q.tag;
   if(q.op==NS_CANCEL){if(!job&&tag&&q.index==app)continue;if(!job||q.index!=app){end_job(job);return 1;}end_job(job);job=0;close(payload);payload=-1;unsigned code=committed(app)?0:ECANCELED;if(emit(NS_DONE,tag,app,code?0:total,total,code))return 1;continue;}
   if(job){end_job(job);return 1;}app=q.index;tag=q.tag;previous=~0U;
   if(!(available&(1U<<app))){if(emit(NS_DONE,tag,app,0,0,ENOTSUP))return 1;continue;}
   if(index.expires<=(uint64_t)time(NULL)){if(emit(NS_DONE,tag,app,0,0,ETIME))return 1;continue;}
   char existing[128];struct stat es;snprintf(existing,sizeof existing,"/home/nova/.nova-apps/%s",nova_catalog[app].id);if(!lstat(existing,&es)||errno!=ENOENT){if(emit(NS_DONE,tag,app,0,0,EEXIST))return 1;continue;}
   total=(unsigned)index.entries[map[app]].bytes;started=seconds_now();payload=(int)syscall(SYS_memfd_create,"nova-store-download",MFD_CLOEXEC|MFD_ALLOW_SEALING|0x8U); /* MFD_NOEXEC_SEAL: download staging cannot execute */if(payload<0){if(emit(NS_DONE,tag,app,0,total,(unsigned)errno))return 1;continue;}
   pid_t parent=getpid();job=fork();if(!job){if(prctl(PR_SET_PDEATHSIG,SIGTERM)||getppid()!=parent||setsid()<0)_exit(EIO);int result=nr_https_fetch(&index.entries[map[app]],"/etc/ssl/certs/ca-certificates.crt",payload);if(!result)result=install_package(payload,app);_exit(result>255?EIO:result);}if(job<0){close(payload);payload=-1;job=0;if(emit(NS_DONE,tag,app,0,total,EAGAIN))return 1;}
  }
  if(job&&seconds_now()-started>=60){end_job(job);job=0;close(payload);payload=-1;unsigned code=committed(app)?0:ETIMEDOUT;if(emit(NS_DONE,tag,app,code?0:total,total,code))return 1;}
  if(job){struct stat st;unsigned bytes=!fstat(payload,&st)&&st.st_size>=0&&(uint64_t)st.st_size<=total?(unsigned)st.st_size:0;if(bytes!=previous){if(!emit(NS_PROGRESS,tag,app,bytes,total,0))previous=bytes;}
   int status;pid_t done=waitpid(job,&status,WNOHANG);if(done==job){job=0;close(payload);payload=-1;unsigned code=WIFEXITED(status)?(unsigned)WEXITSTATUS(status):EIO;if(emit(NS_DONE,tag,app,code?0:total,total,code))return 1;}
  }
 }
}
