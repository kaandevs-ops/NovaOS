#define _GNU_SOURCE
#include "nova_net.h"
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/random.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
int main(int argc,char **argv){
 signal(SIGPIPE,SIG_IGN);
 struct nova_net_url url;if(argc!=2||nova_net_parse(argv[1],&url)){fputs("Usage: nova-http http://public-host/path\nPlain HTTP only; no TLS, redirects or private-address access. Max body 64KiB.\n",stderr);return 2;}
 if(getuid()!=12004||geteuid()!=12004){fputs("nova-http: terminal user required\n",stderr);return 1;}
 struct stat cap;if(fstat(NOVA_NET_FD,&cap)||!S_ISSOCK(cap.st_mode)||cap.st_uid||cap.st_gid){fputs("nova-http: no inherited network capability\n",stderr);return 1;}
 int home=open("/home/nova",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(home<0)return 1;
 int lock=openat(home,".nova-net.lock",O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600);close(home);struct stat st;
 if(lock<0||fstat(lock,&st)||!S_ISREG(st.st_mode)||st.st_uid!=12004||st.st_nlink!=1||(st.st_mode&0777)!=0600){fputs("nova-http: unsafe request lock\n",stderr);return 1;}
 struct flock fl={.l_type=F_WRLCK,.l_whence=SEEK_SET};if(fcntl(lock,F_SETLKW,&fl))return 1;
 unsigned char request[sizeof(struct nova_net_header)+NOVA_NET_URL];struct nova_net_header h={.magic=NOVA_NET_MAGIC,.version=1,.length=strlen(argv[1])};
 if(getrandom(&h.tag,sizeof h.tag,0)!=sizeof h.tag||!h.tag)h.tag=(uint32_t)getpid();
 memcpy(request,&h,sizeof h);memcpy(request+sizeof h,argv[1],h.length);
 ssize_t n;do n=write(NOVA_NET_FD,request,sizeof h+h.length);while(n<0&&errno==EINTR);
 if(n!=(ssize_t)(sizeof h+h.length)){fputs("nova-http: network capability unavailable\n",stderr);return 1;}
 unsigned char *reply=malloc(sizeof h+NOVA_NET_BODY+1);if(!reply)return 1;
 do n=read(NOVA_NET_FD,reply,sizeof h+NOVA_NET_BODY+1);while(n<0&&errno==EINTR);
 struct nova_net_header rh;if(n<(ssize_t)sizeof rh){fputs("nova-http: broker disconnected\n",stderr);return 1;}memcpy(&rh,reply,sizeof rh);
 if(rh.magic!=h.magic||rh.version!=1||rh.tag!=h.tag||rh.length>NOVA_NET_BODY||n!=(ssize_t)(sizeof rh+rh.length)){fputs("nova-http: invalid broker response\n",stderr);return 1;}
 if(rh.code!=200){if(rh.code<0)fprintf(stderr,"nova-http: %s (%d)\n",strerror(-rh.code),rh.code);
  else fprintf(stderr,"nova-http: HTTP status %d; redirects are not followed\n",rh.code);return 1;}
 unsigned char *body=reply+sizeof rh;
 if(isatty(1))for(uint32_t i=0;i<rh.length;i++)if((body[i]<32&&body[i]!='\n'&&body[i]!='\r'&&body[i]!='\t')||body[i]==127)body[i]='.';
 size_t done=0;while(done<rh.length){n=write(1,body+done,rh.length-done);if(n<0&&errno==EINTR)continue;if(n<=0)return 1;done+=(size_t)n;}
 free(reply);close(lock);return 0;
}
