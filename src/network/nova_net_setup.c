#define _GNU_SOURCE
/* Signed, opt-in VirtualBox NAT TEST provisioning. Not DHCP or general host setup. */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
static void address(struct sockaddr *out,const char *text){struct sockaddr_in *a=(struct sockaddr_in*)out;memset(a,0,sizeof *a);a->sin_family=AF_INET;inet_pton(AF_INET,text,&a->sin_addr);}
int main(int argc,char **argv){
 (void)argv;if(argc!=1||getuid()!=0||getppid()!=1)return 126;
 int c=open("/System/Network/etc/test-nat.enabled",O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if(c<0)return errno==ENOENT?2:1;
 struct stat st;char marker[64];ssize_t n=read(c,marker,sizeof marker);int invalid=fstat(c,&st)||!S_ISREG(st.st_mode)||st.st_uid||st.st_gid||st.st_nlink!=1||(st.st_mode&07777)!=0444;close(c);
 const char expected[]="virtualbox-nat-test-v1\n";if(invalid||n!=sizeof expected-1||memcmp(marker,expected,sizeof expected-1))return 1;
 int fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);if(fd<0)return 1;
 struct if_nameindex *names=if_nameindex();if(!names){close(fd);return 1;}char selected[IFNAMSIZ]={0};unsigned matches=0;
 for(struct if_nameindex *p=names;p->if_index;p++){
  struct ifreq q={0};snprintf(q.ifr_name,sizeof q.ifr_name,"%s",p->if_name);
  if(ioctl(fd,SIOCGIFHWADDR,&q))continue;unsigned char *mac=(unsigned char*)q.ifr_hwaddr.sa_data;
  if(q.ifr_hwaddr.sa_family==1&&mac[0]==0x08&&mac[1]==0x00&&mac[2]==0x27){snprintf(selected,sizeof selected,"%s",p->if_name);matches++;}
 }if_freenameindex(names);if(matches!=1){close(fd);return 1;}
 struct ifreq q={0};strcpy(q.ifr_name,selected);if(ioctl(fd,SIOCGIFFLAGS,&q)){close(fd);return 1;}short original=q.ifr_flags;
 /* Never reconfigure a pre-existing address. This helper is only for a fresh NIC. */
 if(ioctl(fd,SIOCGIFADDR,&q)==0&&((struct sockaddr_in*)&q.ifr_addr)->sin_addr.s_addr){close(fd);return 1;}
 strcpy(q.ifr_name,selected);address(&q.ifr_addr,"10.0.2.15");if(ioctl(fd,SIOCSIFADDR,&q))goto fail;
 address(&q.ifr_netmask,"255.255.255.0");if(ioctl(fd,SIOCSIFNETMASK,&q))goto fail;
 q.ifr_flags=original|IFF_UP;if(ioctl(fd,SIOCSIFFLAGS,&q))goto fail;
 struct rtentry route={0};address(&route.rt_dst,"0.0.0.0");address(&route.rt_genmask,"0.0.0.0");address(&route.rt_gateway,"10.0.2.2");route.rt_flags=RTF_UP|RTF_GATEWAY;route.rt_dev=selected;
 if(ioctl(fd,SIOCADDRT,&route))goto fail;
 printf("[nova-network] test_nat=READY interface=%s address=10.0.2.15 gateway=10.0.2.2 dns=10.0.2.3 mode=static-test-only\n",selected);close(fd);return 0;
fail:
 address(&q.ifr_addr,"0.0.0.0");(void)ioctl(fd,SIOCSIFADDR,&q);q.ifr_flags=original;(void)ioctl(fd,SIOCSIFFLAGS,&q);close(fd);return 1;
}
