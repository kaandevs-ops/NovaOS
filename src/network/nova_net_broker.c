#define _GNU_SOURCE
#include "nova_net.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <poll.h>
#include <grp.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifndef NOVA_NET_ROOT
#define NOVA_NET_ROOT "/System/Network"
#endif
#include "nova_net_policy.h"
#define WIRE_MAX (NOVA_NET_BODY+16384U)
static volatile sig_atomic_t expired;
static void deadline(int sig){(void)sig;expired=1;}
static int fetch(const char *text,unsigned char *body,size_t *length){
 struct nova_net_url url;if(nova_net_parse(text,&url))return -EINVAL;
 struct addrinfo hint={.ai_family=AF_INET,.ai_socktype=SOCK_STREAM},*all=NULL;
 if(getaddrinfo(url.host,"80",&hint,&all))return expired?-ETIMEDOUT:-EHOSTUNREACH;
 if(expired){freeaddrinfo(all);return -ETIMEDOUT;}
 int fd=-1,rc=-ENETUNREACH;
 for(struct addrinfo *p=all;p;p=p->ai_next){
  if(expired){rc=-ETIMEDOUT;break;}
  if(p->ai_family!=AF_INET||p->ai_addrlen!=sizeof(struct sockaddr_in))continue;
  struct sockaddr_in ip=*(struct sockaddr_in*)p->ai_addr;
  if(!nova_net_public(ntohl(ip.sin_addr.s_addr))){rc=-EACCES;continue;}
  fd=socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,0);if(fd<0){rc=-errno;break;}
  int ok=connect(fd,(struct sockaddr*)&ip,sizeof ip);
  if(ok<0&&errno==EINPROGRESS){
   struct pollfd wait={fd,POLLOUT,0};int waited=poll(&wait,1,3000),err=0;socklen_t z=sizeof err;
   if(waited==0)err=ETIMEDOUT;
   else if(waited<0)err=expired?ETIMEDOUT:errno;
   else if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&err,&z)<0)err=errno;
   ok=err?-1:0;if(err)rc=-err;
  }else if(ok<0)rc=-errno;
  if(ok==0)break;close(fd);fd=-1;
 }
 freeaddrinfo(all);if(fd<0)return rc;
 if(fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)&~O_NONBLOCK)){close(fd);return -EIO;}
 struct timeval timeout={.tv_sec=3};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof timeout);
 char request[1400];int n=snprintf(request,sizeof request,"GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\nUser-Agent: NovaOS-Network-Foundation/1\r\n\r\n",url.path,url.host);
 if(n<=0||(size_t)n>=sizeof request){close(fd);return -EINVAL;}
 size_t sent=0;while(sent<(size_t)n){ssize_t w=send(fd,request+sent,(size_t)n-sent,MSG_NOSIGNAL);if(w<0&&errno==EINTR&&!expired)continue;if(w<=0){close(fd);return -EIO;}sent+=(size_t)w;}
 unsigned char *wire=malloc(WIRE_MAX+1);if(!wire){close(fd);return -ENOMEM;}size_t used=0;
 for(;;){if(used==WIRE_MAX){rc=-EFBIG;break;}ssize_t got=recv(fd,wire+used,WIRE_MAX-used,0);if(got<0&&errno==EINTR&&!expired)continue;if(got<0){rc=-EIO;break;}if(!got){rc=0;break;}used+=(size_t)got;}
 close(fd);wire[used]=0;if(rc){free(wire);return rc;}
 unsigned char *split=(unsigned char*)strstr((char*)wire,"\r\n\r\n");
 if(!split||(size_t)(split-wire)>16384){free(wire);return -EPROTO;}
 size_t headers=(size_t)(split-wire)+4,bytes=used-headers;*split=0;
 if(used<12||(memcmp(wire,"HTTP/1.0 ",9)&&memcmp(wire,"HTTP/1.1 ",9))||wire[9]<'1'||wire[9]>'5'||wire[10]<'0'||wire[10]>'9'||wire[11]<'0'||wire[11]>'9'||(wire[12]&&wire[12]!=' '&&wire[12]!='\r')){free(wire);return -EPROTO;}
 int status=(wire[9]-'0')*100+(wire[10]-'0')*10+wire[11]-'0';
 if(status!=200){free(wire);return status;}
 if(strcasestr((char*)wire,"\r\nTransfer-Encoding:")){free(wire);return -ENOTSUP;}
 const char *cl=strcasestr((char*)wire,"\r\nContent-Length:");
 if(cl){char *end;errno=0;unsigned long count=strtoul(cl+17,&end,10);if(errno||end==cl+17||(*end!='\r'&&*end!=0)||strcasestr(end,"\r\nContent-Length:")||count!=bytes){free(wire);return -EPROTO;}}
 if(bytes>NOVA_NET_BODY){free(wire);return -EFBIG;}memcpy(body,wire+headers,bytes);*length=bytes;free(wire);return 200;
}
int main(int argc,char **argv){
 (void)argv;struct ucred peer;socklen_t sz=sizeof peer;int type;socklen_t tz=sizeof type;
 if(argc!=1||getuid()!=0||getsockopt(3,SOL_SOCKET,SO_TYPE,&type,&tz)||type!=SOCK_SEQPACKET||getsockopt(3,SOL_SOCKET,SO_PEERCRED,&peer,&sz)||peer.uid||peer.pid!=getppid())return 126;
 if(close_range(4,~0U,0))return 1;
 int root=open(NOVA_NET_ROOT,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);struct stat st;
 if(root<0||fstat(root,&st)||st.st_uid||st.st_gid||(st.st_mode&0022)||fchdir(root)||chroot(".")||chdir("/"))return 1;close(root);
 int etc=open("/etc",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);struct stat es;
 if(etc<0||fstat(etc,&es)||es.st_uid||es.st_gid||(es.st_mode&0022))return 1;
 int grant=openat(etc,"http-enabled",O_RDONLY|O_NOFOLLOW|O_CLOEXEC);close(etc);char token[64];const char expected[]="http-public-read-v1\n";
 if(grant<0)return 2;ssize_t count=read(grant,token,sizeof token);struct stat gs;
 int badgrant=fstat(grant,&gs)||!S_ISREG(gs.st_mode)||gs.st_uid||gs.st_gid||gs.st_nlink!=1||(gs.st_mode&07777)!=0444;close(grant);
 if(badgrant||count!=sizeof expected-1||memcmp(token,expected,sizeof expected-1))return 1;
 int one=1;if(setsockopt(3,SOL_SOCKET,SO_PASSCRED,&one,sizeof one))return 1;
 struct timeval send_limit={.tv_sec=3};if(setsockopt(3,SOL_SOCKET,SO_SNDTIMEO,&send_limit,sizeof send_limit))return 1;
 struct rlimit mem={64UL*1024*1024,64UL*1024*1024},files={32,32},cpu={30,30},core={0,0};
 if(setrlimit(RLIMIT_AS,&mem)||setrlimit(RLIMIT_NOFILE,&files)||setrlimit(RLIMIT_CPU,&cpu)||setrlimit(RLIMIT_CORE,&core)||setgroups(0,NULL)||setgid(12005)||setuid(12005)||prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)||prctl(PR_SET_DUMPABLE,0)||prctl(PR_SET_PDEATHSIG,SIGKILL))return 1;
 signal(SIGPIPE,SIG_IGN);struct sigaction action={.sa_handler=deadline};sigemptyset(&action.sa_mask);if(sigaction(SIGALRM,&action,NULL))return 1;
 if(net_policy())return 1;
 struct nova_net_header ready={.magic=NOVA_NET_MAGIC,.version=1};if(send(3,&ready,sizeof ready,MSG_NOSIGNAL)!=sizeof ready)return 1;
 for(;;){
  unsigned char request[sizeof(struct nova_net_header)+NOVA_NET_URL+1];unsigned char control[CMSG_SPACE(sizeof(struct ucred))+CMSG_SPACE(16*sizeof(int))];
  struct iovec io={request,sizeof request};struct msghdr msg={.msg_iov=&io,.msg_iovlen=1,.msg_control=control,.msg_controllen=sizeof control};
  ssize_t got;do got=recvmsg(3,&msg,0);while(got<0&&errno==EINTR);if(got<=0)return 0;
  unsigned creds=0,bad=0;struct ucred who={0};
  for(struct cmsghdr *c=CMSG_FIRSTHDR(&msg);c;c=CMSG_NXTHDR(&msg,c)){
   if(c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_RIGHTS){size_t n=(c->cmsg_len-CMSG_LEN(0))/sizeof(int);int *fds=(int*)CMSG_DATA(c);for(size_t i=0;i<n;i++)close(fds[i]);bad=1;}
   else if(c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_CREDENTIALS&&c->cmsg_len==CMSG_LEN(sizeof who)){memcpy(&who,CMSG_DATA(c),sizeof who);creds++;}else bad=1;
  }
  if(bad||creds!=1||who.uid!=12004||who.gid!=12004||who.pid<=0||(msg.msg_flags&(MSG_TRUNC|MSG_CTRUNC))||got<(ssize_t)sizeof(struct nova_net_header))return 1;
  struct nova_net_header h;memcpy(&h,request,sizeof h);
  if(h.magic!=NOVA_NET_MAGIC||h.version!=1||!h.tag||h.code||!h.length||h.length>NOVA_NET_URL||got!=(ssize_t)(sizeof h+h.length)||memchr(request+sizeof h,0,h.length))return 1;
  request[sizeof h+h.length]=0;
  unsigned char reply[sizeof h+NOVA_NET_BODY];size_t bytes=0;expired=0;alarm(12);int code=fetch((char*)request+sizeof h,reply+sizeof h,&bytes);alarm(0);
  h.code=code;h.length=(uint32_t)bytes;memcpy(reply,&h,sizeof h);
  ssize_t written=send(3,reply,sizeof h+bytes,MSG_NOSIGNAL);if(written!=(ssize_t)(sizeof h+bytes))return 0;
 }
}
