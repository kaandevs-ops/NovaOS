#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/mman.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nova_browser_frame.h"
int main(int n,char**a){if(n!=3)return 2;int sock=atoi(a[2]),type=0;socklen_t len=sizeof type;if(getsockopt(sock,SOL_SOCKET,SO_TYPE,&type,&len)||type!=SOCK_SEQPACKET)return 3;struct nova_x11*x=nova_x11_open(a[1]);if(!x)return 4;struct nova_browser_frame f;assert(!nova_browser_frame_create(x,&f));assert(!nova_browser_frame_capture(x,&f));assert(nova_x11_pixels(x,f.pixels,f.bytes-1)<0);assert(nova_x11_key(x,0,1)<0&&nova_x11_pointer(x,-1,0,1,1)<0);int seals=fcntl(f.fd,F_GET_SEALS);assert((seals&(F_SEAL_GROW|F_SEAL_SHRINK))==(F_SEAL_GROW|F_SEAL_SHRINK));assert(ftruncate(f.fd,0)<0);struct nova_surface_message msg;assert(!nova_browser_frame_message(&f,4,1,1,&msg));assert(nova_browser_frame_message(&f,0,1,1,&msg)<0);assert(!nova_browser_frame_message(&f,4,1,1,&msg));struct iovec io={&msg,sizeof msg};char control[CMSG_SPACE(sizeof(int))]={0};struct msghdr hdr={.msg_iov=&io,.msg_iovlen=1,.msg_control=control,.msg_controllen=sizeof control};struct cmsghdr*c=CMSG_FIRSTHDR(&hdr);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&f.fd,sizeof f.fd);assert(sendmsg(sock,&hdr,MSG_NOSIGNAL)==sizeof msg);nova_browser_frame_close(&f);nova_x11_close(x);puts("ACTUAL_X11_FRAME_NATIVE_V6_METADATA_MEMFD_SCM_RIGHTS_BOUNDS_SEALS_PASS");return 0;}
