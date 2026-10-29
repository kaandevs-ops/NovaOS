#include "nova_desktop_open.h"
#include "nova_theme.h"
#include "nova_input.h"
#include "nova_runtime.h"
#include "nova_files_grant.h"
#include "nova_files_guest_adapter.h"
#include "nova_files_guest_metadata.h"
#include "nova_files_home_client.h"
#include "nova_files_surface_bridge.h"
#ifndef NOVA_FILES_SOCKET_PATH
#define NOVA_FILES_SOCKET_PATH NOVA_SURFACE_SOCKET_PATH
#endif
#define W NOVA_FILES_UI_WIDTH
#define H NOVA_FILES_UI_HEIGHT
#define BYTES (W*H*4U)
static struct nova_files_model model;
static struct nova_files_ui ui;
static struct nova_files_surface_bridge bridge;
static unsigned int sequence=1;
static void message(struct nova_surface_message *m,unsigned int op) {
 nova_zero(m,sizeof(*m)); m->magic=NOVA_SURFACE_PROTOCOL_MAGIC;
 m->version=NOVA_SURFACE_PROTOCOL_VERSION; m->size=sizeof(*m);
 m->client_id=3; m->surface_id=300; m->sequence=sequence++; m->opcode=op;
}
static int send_message(int fd,struct nova_surface_message *m,int pass) {
 if(pass<0)return nova_write(fd,m,sizeof(*m))==(long)sizeof(*m);
 struct nova_iovec iov={m,sizeof(*m)};struct nova_msghdr msg;
 union {struct nova_cmsghdr align;unsigned char bytes[NOVA_CMSG_SPACE(sizeof(int))];} ctl;
 nova_zero(&msg,sizeof(msg));nova_zero(&ctl,sizeof(ctl));
 msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=ctl.bytes;msg.msg_controllen=sizeof(ctl);
 struct nova_cmsghdr *c=(struct nova_cmsghdr*)ctl.bytes;
 c->cmsg_len=NOVA_CMSG_LEN(sizeof(int));c->cmsg_level=NOVA_SOL_SOCKET;c->cmsg_type=NOVA_SCM_RIGHTS;
 nova_copy(ctl.bytes+NOVA_CMSG_DATA_OFFSET,&pass,sizeof(pass));
 return nova_sendmsg(fd,&msg,0)==(long)sizeof(*m);
}
static int simple(int fd,unsigned int op) {
 struct nova_surface_message m;message(&m,op);return send_message(fd,&m,-1);
}
struct files_frame {int fd;void *pixels;unsigned int id,w,h;unsigned long bytes;};
static void release_frame(struct files_frame*f){
 if(f->pixels)nova_munmap(f->pixels,f->bytes);if(f->fd>=0)nova_close(f->fd);
 f->pixels=0;f->fd=-1;
}
static int create_frame(struct files_frame*f,unsigned int id,unsigned int w,unsigned int h){
 nova_zero(f,sizeof(*f));f->fd=-1;
 if(w<640U||h<400U||w>1920U||h>1080U)return 0;
 f->id=id;f->w=w;f->h=h;f->bytes=(unsigned long)w*h*4U;
 if(f->bytes>NOVA_SURFACE_MAX_BUFFER_BYTES)return 0;
 long n=nova_memfd_create("nova-files-pixels",NOVA_MFD_CLOEXEC);if(n<0)return 0;f->fd=(int)n;
 if(nova_ftruncate(f->fd,f->bytes)<0){release_frame(f);return 0;}
 f->pixels=nova_mmap(0,f->bytes,NOVA_PROT_READ|NOVA_PROT_WRITE,NOVA_MAP_SHARED,f->fd,0);
 if((unsigned long)f->pixels>=(unsigned long)-4095L){f->pixels=0;release_frame(f);return 0;}
 return 1;
}
static int register_frame(int fd,const struct files_frame*f){
 struct nova_surface_message m;message(&m,NOVA_SURFACE_OP_CREATE_BUFFER);m.surface_id=0;
 m.buffer_id=f->id;m.width=f->w;m.height=f->h;m.stride=f->w*4U;m.buffer_bytes=f->bytes;
 m.pixel_format=NOVA_PIXEL_FORMAT_XRGB8888;return send_message(fd,&m,f->fd);
}
static int destroy_frame_remote(int fd,unsigned int id){
 struct nova_surface_message m;message(&m,NOVA_SURFACE_OP_DESTROY_BUFFER);m.surface_id=0;m.buffer_id=id;
 return send_message(fd,&m,-1);
}
static int attach_frame(int fd,const struct files_frame*f){
 struct nova_surface_message m;message(&m,NOVA_SURFACE_OP_ATTACH_BUFFER);m.buffer_id=f->id;
 return send_message(fd,&m,-1);
}
static int repaint(int fd,const struct files_frame*f){
 if(home_client.shared&&home_client.socket<0)ui.last_error=(unsigned)-NOVA_FILES_IO;
 if(!nova_files_ui_render(&ui,f->pixels,f->w,f->h,f->w*4U))return 0;
 struct nova_surface_message m;message(&m,NOVA_SURFACE_OP_DAMAGE);m.damage_width=f->w;m.damage_height=f->h;
 return send_message(fd,&m,-1)&&simple(fd,NOVA_SURFACE_OP_COMMIT);
}
static int open_window(int fd,const struct files_frame*f){
 struct nova_surface_message m;message(&m,NOVA_SURFACE_OP_CREATE_SURFACE);
 m.x=80;m.y=100;m.width=f->w;m.height=f->h;m.z_index=40;
 if(!send_message(fd,&m,-1))return 0;
 message(&m,NOVA_SURFACE_OP_SET_ROLE);m.role=NOVA_SURFACE_ROLE_TOPLEVEL;if(!send_message(fd,&m,-1))return 0;
 message(&m,NOVA_SURFACE_OP_SET_TITLE);nova_copy(m.title,"Nova Files",11);if(!send_message(fd,&m,-1))return 0;
 message(&m,NOVA_SURFACE_OP_SET_SIZE_LIMITS);m.min_width=640;m.min_height=400;m.max_width=1920;m.max_height=1080;
 if(!send_message(fd,&m,-1)||!attach_frame(fd,f))return 0;
 nova_files_surface_bridge_init(&bridge,&ui,3,300,80,100);return repaint(fd,f);
}
static int poll_shared_home(void){
 static unsigned last_token;static unsigned long long last_stamp;
 if(!home_client.shared)return 0;
 if(home_client.socket<0){if(!ui.last_error){ui.last_error=(unsigned)-NOVA_FILES_IO;return 1;}return 0;}
 int current=model.directory_fds[model.depth],pass=-1;unsigned token=home_token(current);struct nova_home_directory_reply reply;
 if(home_request(NOVA_HOME_DIRECTORY_STAMP,token,0,&reply,&pass)<0){ui.last_error=(unsigned)-NOVA_FILES_IO;return 1;}
 if(last_token!=token){last_token=token;last_stamp=reply.bytes;(void)nova_files_ui_key(&ui,'r');return 1;}
 if(last_stamp!=reply.bytes){last_stamp=reply.bytes;(void)nova_files_ui_key(&ui,'r');return 1;}
 return nova_files_ui_poll_metadata(&ui);
}
int nova_main(void) {
 /* FD 3 is a one-shot trusted novad launch channel, never a directory fallback.
  * Authenticate peer/scope and type-check the SCM_RIGHTS directory before UI IPC. */
 int directory=nova_files_receive_home_grant(NOVA_FILES_GRANT_SOCKET_FD);
 nova_close(NOVA_FILES_GRANT_SOCKET_FD);
 if(directory<0)return 21;
 if(nova_files_init(&model,directory,&nova_files_home_ops,0)!=NOVA_FILES_OK){nova_close(directory);return 20;}
 if(nova_files_list(&model)!=NOVA_FILES_OK){nova_files_close(&model);return 21;}
 nova_files_ui_init(&ui,&model);ui.preview_reader=nova_files_home_preview;ui.workspace_shared=(unsigned)home_client.shared;nova_files_ui_set_metadata(&ui,nova_files_home_metadata);if(home_client.shared)(void)poll_shared_home();
 int result=22,fd=-1;struct files_frame frame;frame.fd=-1;frame.pixels=0;
 if(!create_frame(&frame,1,W,H))goto done;
 long n=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0);if(n<0)goto done;fd=(int)n;
 struct nova_sockaddr_un addr;nova_zero(&addr,sizeof(addr));addr.sun_family=NOVA_AF_UNIX;
 _Static_assert(sizeof(NOVA_FILES_SOCKET_PATH)<=sizeof(addr.sun_path),"socket path too long");
 nova_copy(addr.sun_path,NOVA_FILES_SOCKET_PATH,sizeof(NOVA_FILES_SOCKET_PATH));
 if(nova_connect(fd,&addr,sizeof(addr))<0)goto done;
 struct nova_surface_message m;message(&m,NOVA_SURFACE_OP_HELLO);m.surface_id=0;m.input_flags=NOVA_SURFACE_FEATURE_THEME|NOVA_SURFACE_FEATURE_DESKTOP_OPEN;
 if(!send_message(fd,&m,-1))goto done;
 if(!register_frame(fd,&frame)||!open_window(fd,&frame))goto done;
 for(;;){
  struct nova_pollfd pollfd={fd,NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP,0};
  struct nova_timespec timeout={1,0};long ready=nova_poll(&pollfd,1,&timeout);
  if(ready==-NOVA_EINTR)continue;
  if(ready<0||(pollfd.revents&(NOVA_POLLERR|NOVA_POLLHUP)))break;
  if(!ready){if(poll_shared_home()&&!repaint(fd,&frame))break;if(!simple(fd,NOVA_SURFACE_OP_HEARTBEAT))break;continue;}
  struct nova_surface_event ev;
  struct nova_iovec iov={&ev,sizeof(ev)};struct nova_msghdr incoming;
  nova_zero(&incoming,sizeof(incoming));incoming.msg_iov=&iov;incoming.msg_iovlen=1;
  long got=nova_recvmsg(fd,&incoming,NOVA_MSG_TRUNC);
  if(got!=(long)sizeof(ev)||(incoming.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)))break;
  char drag_path[NOVA_SURFACE_CLIPBOARD_BYTES];int offer=0;
  if(ev.magic==NOVA_SURFACE_PROTOCOL_MAGIC&&ev.version==NOVA_SURFACE_PROTOCOL_VERSION&&ev.size==sizeof ev&&ev.client_id==3&&ev.surface_id==300&&ev.type==NOVA_SURFACE_EVENT_POINTER&&ev.code==NOVA_BTN_LEFT&&ev.value==1)
   offer=nova_files_ui_path_at(&ui,ev.cursor_x-bridge.surface_x,ev.cursor_y-bridge.surface_y,drag_path,sizeof drag_path);
  int changed=nova_files_surface_bridge_event(&bridge,&ev);
  if(ev.type==NOVA_SURFACE_EVENT_DESKTOP_OPEN&&(changed==1||changed==-1))nova_putln(changed==1?"[nova-files] desktop_open=APPLIED":"[nova-files] desktop_open=ERROR");
  if(offer&&changed!=-2){message(&m,NOVA_SURFACE_OP_DRAG_PATH);m.clipboard_length=(unsigned)nova_strlen(drag_path);nova_copy(m.clipboard,drag_path,m.clipboard_length+1);if(!send_message(fd,&m,-1))break;}

  if(changed==-2)continue;
  if(ev.type==NOVA_SURFACE_EVENT_CLOSE_REQUEST){
   if(!simple(fd,NOVA_SURFACE_OP_DESTROY_SURFACE))break;
   if(!destroy_frame_remote(fd,frame.id))break;
   result=0;break; /* novad must cold-launch a fresh process on reopen */
  }
  if(ev.type==NOVA_SURFACE_EVENT_CONFIGURE){
   if(changed!=1||!ev.configure_serial)continue;
   if(ev.width!=frame.w||ev.height!=frame.h){
    struct files_frame fresh;
    if(!create_frame(&fresh,frame.id+1U,ev.width,ev.height))break;
    if(!register_frame(fd,&fresh)){release_frame(&fresh);break;}
    message(&m,NOVA_SURFACE_OP_ACK_CONFIGURE);m.configure_serial=ev.configure_serial;
    if(!send_message(fd,&m,-1)||!attach_frame(fd,&fresh)||!repaint(fd,&fresh)){
     release_frame(&fresh);break;
    }
    unsigned int old=frame.id;release_frame(&frame);frame=fresh;
    if(!destroy_frame_remote(fd,old))break;
    continue;
   }
   message(&m,NOVA_SURFACE_OP_ACK_CONFIGURE);m.configure_serial=ev.configure_serial;
   if(!send_message(fd,&m,-1))break;
  }
  /* Accepted UI failures also change the visible error state. Invalid peer events (-2) stay ignored. */
  if((changed==1||changed==-1)&&!repaint(fd,&frame))break;
 }
 done:
 if(fd>=0)(void)nova_close(fd);
 release_frame(&frame);
 nova_files_close(&model);return result;
}
