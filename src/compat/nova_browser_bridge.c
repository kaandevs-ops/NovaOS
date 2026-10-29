#define _GNU_SOURCE
#include "nova_browser_bridge.h"
#include "nova_browser_frame.h"
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#define CLIENT 4U
#define SURFACE 400U
struct bridge {int fd;int lifecycle;unsigned seq;struct nova_x11*x;struct nova_browser_frame frame;int x0,y0,focus;unsigned last_event,modifiers;};
static struct nova_surface_message message(struct bridge*b,unsigned op){
 struct nova_surface_message m={.magic=NOVA_SURFACE_PROTOCOL_MAGIC,.version=6,.opcode=op,.size=sizeof(m),.client_id=CLIENT,.surface_id=SURFACE,.sequence=b->seq++};return m;
}
static int send_wire(struct bridge*b,struct nova_surface_message*m,int fd){
 if(!m->sequence)return -1;
 struct iovec iov={m,sizeof(*m)};union {struct cmsghdr align;char bytes[CMSG_SPACE(sizeof(int))];}control;
 struct msghdr msg={.msg_iov=&iov,.msg_iovlen=1};
 if(fd>=0){memset(&control,0,sizeof(control));msg.msg_control=control.bytes;msg.msg_controllen=sizeof(control.bytes);struct cmsghdr*c=CMSG_FIRSTHDR(&msg);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));memcpy(CMSG_DATA(c),&fd,sizeof(fd));}
 ssize_t n;do n=sendmsg(b->fd,&msg,MSG_NOSIGNAL|MSG_DONTWAIT);while(n<0&&errno==EINTR);return n==(ssize_t)sizeof(*m)?0:-1;
}
static int simple(struct bridge*b,unsigned op){struct nova_surface_message m=message(b,op);return send_wire(b,&m,-1);}
static int present(struct bridge*b){
 struct nova_surface_message m=message(b,NOVA_SURFACE_OP_DAMAGE);m.damage_width=b->frame.width;m.damage_height=b->frame.height;
 return send_wire(b,&m,-1)||simple(b,NOVA_SURFACE_OP_COMMIT)?-1:0;
}
/* Surface v6 carries authoritative Ctrl/Shift state: the compositor consumes
 * physical modifier events rather than forwarding them as ordinary keys. */
static int sync_modifiers(struct bridge*b,unsigned modifiers){
 if(modifiers&~(NOVA_INPUT_MOD_SHIFT|NOVA_INPUT_MOD_CTRL))return -1;
 unsigned changed=b->modifiers^modifiers;
 if((changed&NOVA_INPUT_MOD_CTRL)&&nova_x11_key(b->x,29,!!(modifiers&NOVA_INPUT_MOD_CTRL)))return -1;
 if((changed&NOVA_INPUT_MOD_SHIFT)&&nova_x11_key(b->x,42,!!(modifiers&NOVA_INPUT_MOD_SHIFT)))return -1;
 b->modifiers=modifiers;return 0;
}
static int receive_event(struct bridge*b){
 struct nova_surface_event e;char control[CMSG_SPACE(8*sizeof(int))];struct iovec io={&e,sizeof(e)};
 struct msghdr msg={.msg_iov=&io,.msg_iovlen=1,.msg_control=control,.msg_controllen=sizeof(control)};
 ssize_t n=recvmsg(b->fd,&msg,MSG_CMSG_CLOEXEC|MSG_DONTWAIT);if(n<0&&(errno==EAGAIN||errno==EINTR))return 0;
 int extra=0;for(struct cmsghdr*c=CMSG_FIRSTHDR(&msg);c;c=CMSG_NXTHDR(&msg,c)){
  extra=1;if(c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_RIGHTS&&c->cmsg_len>=CMSG_LEN(0)){size_t count=(c->cmsg_len-CMSG_LEN(0))/sizeof(int);for(size_t i=0;i<count;i++){int fd;memcpy(&fd,(char*)CMSG_DATA(c)+i*sizeof(fd),sizeof(fd));close(fd);}}
 }
 if(n!=(ssize_t)sizeof(e)||extra||(msg.msg_flags&(MSG_TRUNC|MSG_CTRUNC))||e.magic!=NOVA_SURFACE_PROTOCOL_MAGIC||e.version!=6||e.size!=sizeof(e)||e.client_id!=CLIENT||e.surface_id!=SURFACE||!e.sequence||e.sequence<=b->last_event)return -1;
 b->last_event=e.sequence;
 if(e.type==NOVA_SURFACE_EVENT_CLOSE_REQUEST){nova_x11_release_inputs(b->x);(void)nova_x11_request_close(b->x);return b->lifecycle>=0?0:1;}
 if(e.type==NOVA_SURFACE_EVENT_FOCUS){if(e.value!=0&&e.value!=1)return -1;b->focus=e.value;if(!e.value){nova_x11_release_inputs(b->x);b->modifiers=0;}return 0;}
 if(e.type==NOVA_SURFACE_EVENT_CONFIGURE){
  if(!e.configure_serial||e.width!=b->frame.width||e.height!=b->frame.height)return -1;
  b->x0=e.x;b->y0=e.y;struct nova_surface_message m=message(b,NOVA_SURFACE_OP_ACK_CONFIGURE);m.configure_serial=e.configure_serial;return send_wire(b,&m,-1)||present(b)?-1:0;
 }
 if(e.type==NOVA_SURFACE_EVENT_KEY){if(!b->focus)return 0;if(sync_modifiers(b,e.modifiers))return -1;if(e.code==29||e.code==97||e.code==42||e.code==54)return 0;if(e.value==2)return 0;return nova_x11_key(b->x,e.code,e.value);}
 if(e.type==NOVA_SURFACE_EVENT_POINTER){
  if(!b->focus)return 0;
  if(sync_modifiers(b,e.modifiers))return -1;
  int64_t px=(int64_t)e.cursor_x-b->x0,py=(int64_t)e.cursor_y-b->y0-b->frame.header_height;
  /* A captured release outside the content must not leave a stuck button. */
  if(px<0)px=0;if(py<0)py=0;if(px>=b->frame.width)px=b->frame.width-1;if(py>=b->frame.height-b->frame.header_height)py=b->frame.height-b->frame.header_height-1;
  unsigned button=e.code==272?1:e.code==273?3:e.code==274?2:0;
  if(e.code==8){if(e.value<-32||e.value>32)return -1;unsigned wheel=e.value>0?4:5;int count=abs(e.value);while(count--)if(nova_x11_pointer(b->x,(int)px,(int)py,wheel,1)||nova_x11_pointer(b->x,(int)px,(int)py,wheel,0))return -1;return 0;}
  if(!button&&e.code!=0&&e.code!=1)return -1;
  return nova_x11_pointer(b->x,(int)px,(int)py,button,button?e.value:0);
 }
 return -1;
}
static uint64_t now(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return (uint64_t)t.tv_sec*1000000000ULL+(uint64_t)t.tv_nsec;}
int nova_browser_bridge_run_supervised(int fd,const char*display,int lifecycle){
 struct bridge b={.fd=fd,.lifecycle=lifecycle,.seq=1,.frame={.fd=-1},.x0=64,.y0=72};int rc=-1;
 b.x=nova_x11_open(display);if(!b.x)return -1;
 if(nova_browser_frame_create_decorated(b.x,&b.frame)||nova_browser_frame_capture(b.x,&b.frame))goto out;
 if(simple(&b,NOVA_SURFACE_OP_HELLO))goto out;
 struct nova_surface_message m;if(nova_browser_frame_message(&b.frame,CLIENT,1,b.seq++,&m)||send_wire(&b,&m,b.frame.fd))goto out;
 m=message(&b,NOVA_SURFACE_OP_CREATE_SURFACE);m.x=b.x0;m.y=b.y0;m.width=b.frame.width;m.height=b.frame.height;m.z_index=10;
 if(send_wire(&b,&m,-1))goto out;
 m=message(&b,NOVA_SURFACE_OP_SET_ROLE);m.role=NOVA_SURFACE_ROLE_TOPLEVEL;if(send_wire(&b,&m,-1))goto out;
 m=message(&b,NOVA_SURFACE_OP_SET_TITLE);memcpy(m.title,"Firefox",8);if(send_wire(&b,&m,-1))goto out;
 m=message(&b,NOVA_SURFACE_OP_SET_SIZE_LIMITS);m.min_width=m.max_width=b.frame.width;m.min_height=m.max_height=b.frame.height;if(send_wire(&b,&m,-1))goto out;
 m=message(&b,NOVA_SURFACE_OP_SET_INPUT_FLAGS);m.input_flags=NOVA_SURFACE_INPUT_CAPTURE_TAB|NOVA_SURFACE_INPUT_TEXT_ASCII;if(send_wire(&b,&m,-1))goto out;
 m=message(&b,NOVA_SURFACE_OP_ATTACH_BUFFER);m.buffer_id=1;if(send_wire(&b,&m,-1)||present(&b))goto out;
 uint64_t next=now()+33333333;
 for(;;){struct pollfd fds[2]={{.fd=fd,.events=POLLIN},{.fd=lifecycle,.events=POLLIN}};uint64_t t=now();int delay=t>=next?0:(int)((next-t+999999)/1000000);int n=poll(fds,2,delay);if(n<0){if(errno==EINTR)continue;goto out;}if(fds[1].revents){rc=0;break;}if(fds[0].revents&(POLLHUP|POLLERR|POLLNVAL))goto out;
  if(fds[0].revents&POLLIN){int event=receive_event(&b);if(event<0)goto out;if(event==1){rc=0;break;}}
  t=now();if(t>=next){int changed=nova_browser_frame_refresh(b.x,&b.frame);if(changed<0||(changed&&present(&b)))goto out;next=t+33333333;}
 }
 (void)simple(&b,NOVA_SURFACE_OP_DESTROY_SURFACE);m=message(&b,NOVA_SURFACE_OP_DESTROY_BUFFER);m.buffer_id=1;(void)send_wire(&b,&m,-1);
 out:nova_browser_frame_close(&b.frame);nova_x11_close(b.x);return rc;
}

int nova_browser_bridge_run(int fd,const char*display){return nova_browser_bridge_run_supervised(fd,display,-1);}
