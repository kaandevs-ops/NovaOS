#include "nova_keyboard.h"
static unsigned g_terminal_altgr,g_terminal_caps;
#ifndef NOVA_TERMINAL_CLIENT_H
#define NOVA_TERMINAL_CLIENT_H
#include "nova_terminal_grid.h"
#include "nova_runtime.h"
#if defined(__x86_64__)
#define NOVA_TERM_NR_SOCKETPAIR 53
#define NOVA_TERM_NR_DUP3 292
#define NOVA_TERM_NR_FCNTL 72
#else
#define NOVA_TERM_NR_SOCKETPAIR 199
#define NOVA_TERM_NR_DUP3 24
#define NOVA_TERM_NR_FCNTL 25
#endif
struct nova_terminal_desktop {
 struct nova_term_grid grid;
 struct nova_sixel *graphics;
 int fd;long child;
 unsigned visible,ready,ended,error,focused,placed,grab,queued,resize_pending,backpressure;
 int x,y,width,height,grab_x,grab_y;
 unsigned maximized;int restore_x,restore_y,restore_width,restore_height;
 unsigned rows,cols,z_index,persistent;
 unsigned char input[4096];
};
static struct nova_sixel g_terminal_graphics; /* zero-only storage stays in BSS */
static struct nova_terminal_desktop g_terminal={.fd=-1,.graphics=&g_terminal_graphics};
static struct nova_desktop_snapshot g_desktop_files;
static unsigned g_desktop_pending,g_desktop_inflight,g_desktop_selected=~0U;
static char g_desktop_selected_name[64];
static struct nova_desktop_file_request g_desktop_file_request;
static struct nova_desktop_file_reply g_desktop_preview;
static struct nova_desktop_trash_snapshot g_desktop_trash_files;
static unsigned g_desktop_file_pending,g_desktop_file_inflight,g_desktop_file_error,g_desktop_file_last_action;
static unsigned g_desktop_document_visible,g_desktop_trash_visible,g_desktop_trash_selected=~0U,g_desktop_trash_scroll;
static void r25_terminal_focus(void);
static long nova_term_send(unsigned op,const void *data,unsigned size,unsigned rows,unsigned cols){
 struct nova_term_packet p;nova_zero(&p,sizeof p);p.magic=NOVA_TERM_MAGIC;p.version=NOVA_TERM_VERSION;p.op=op;p.length=size;p.rows=rows;p.cols=cols;
 if(size>NOVA_TERM_BYTES)return -22;if(size)nova_copy(p.data,data,size);
 struct nova_iovec io={&p,NOVA_TERM_HEADER+size};struct nova_msghdr m;nova_zero(&m,sizeof m);m.msg_iov=&io;m.msg_iovlen=1;
 return nova_sendmsg(g_terminal.fd,&m,0x40|0x4000); /* DONTWAIT | NOSIGNAL */
}
static void nova_term_disconnect(unsigned error){
 if(g_terminal.fd>=0)nova_close(g_terminal.fd);g_terminal.fd=-1;g_terminal.ready=0;g_terminal.ended=1;g_terminal.error=error;g_terminal.queued=0;g_desktop_inflight=0;g_desktop_file_pending=0;g_desktop_file_inflight=0;g_desktop_file_error=error;
}
static int nova_term_reap(void){
 if(g_terminal.child>0){int status;long n=nova_wait4(g_terminal.child,&status,NOVA_WNOHANG);if(n==g_terminal.child||n==-10){g_terminal.child=0;return 1;}}
 return 0;
}
static int nova_term_start(void){
 nova_term_reap();if(g_terminal.fd>=0||g_terminal.child>0)return 0;
 int fds[2];long rc=nova_syscall4(NOVA_TERM_NR_SOCKETPAIR,NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK|0x80000,0,(long)fds);
 if(rc<0){g_terminal.error=(unsigned)-rc;g_terminal.ended=1;return 0;}
 long pid=nova_clone_process();
 if(pid<0){nova_close(fds[0]);nova_close(fds[1]);g_terminal.error=(unsigned)-pid;g_terminal.ended=1;return 0;}
 if(!pid){
  nova_close(fds[0]);
  if(fds[1]!=3){if(nova_syscall3(NOVA_TERM_NR_DUP3,fds[1],3,0)<0)nova_exit(126);}
  else if(nova_syscall3(NOVA_TERM_NR_FCNTL,3,2,0)<0)nova_exit(126);
  if(nova_close_range(4,~0U,0)<0)nova_exit(126);
  for(int i=0;i<3;i++)nova_close(i);
  char *argv[]={NOVA_TERM_HELPER,(char *)0};char *env[]={(char *)0};nova_execve(argv[0],argv,env);
  struct nova_term_packet e;nova_zero(&e,sizeof e);e.magic=NOVA_TERM_MAGIC;e.version=1;e.op=NOVA_TERM_ERROR;e.code=2;
  (void)nova_write(3,&e,NOVA_TERM_HEADER);nova_exit(127);
 }
 nova_close(fds[1]);g_terminal.fd=fds[0];g_terminal.child=pid;g_terminal.ready=0;g_terminal.persistent=0;g_terminal.ended=0;g_terminal.error=0;g_terminal.queued=0;g_terminal.backpressure=0;g_terminal.rows=24;g_terminal.cols=80;g_terminal.resize_pending=1;
 nova_term_grid_init(&g_terminal.grid,24,80);g_terminal.graphics->visible=0;g_terminal.grid.graphics=g_terminal.graphics;return 1;
}
static void nova_term_toggle(void){
 /* Activation from another app must raise the existing terminal, not hide it.
  * A second invocation while focused still minimizes without killing the PTY. */
 if(g_terminal.visible&&!g_terminal.focused){
  g_terminal.grid.scroll=0;r25_terminal_focus();return;
 }
 g_terminal.visible=!g_terminal.visible;g_terminal.focused=g_terminal.visible;
 if(g_terminal.visible){if(g_terminal.fd<0)nova_term_start();g_terminal.grid.scroll=0;r25_terminal_focus();}
}
static int nova_term_receive(void){
 if(g_terminal.fd<0)return 0;int changed=0;nova_sixel_turn(g_terminal.graphics);
 /* At most four packets / 8 KiB per compositor turn. No output flood starvation. */
 for(unsigned i=0;i<4;i++){
  struct nova_term_packet p;struct nova_iovec io={&p,sizeof p};struct nova_msghdr m;nova_zero(&m,sizeof m);m.msg_iov=&io;m.msg_iovlen=1;
  long n=nova_recvmsg(g_terminal.fd,&m,0x40|0x20); /* DONTWAIT|TRUNC */
  if(n==-11||n==-4)break;
  if(n<=0){nova_term_disconnect(n<0?(unsigned)-n:0);return 1;}
  if(!nova_term_valid(&p,(unsigned long)n,0)||m.msg_controllen||m.msg_flags){nova_term_disconnect(71);return 1;}
  if(p.op==NOVA_TERM_DESKTOP_STATE){
   struct nova_desktop_snapshot state;nova_copy(&state,p.data,sizeof state);
   int valid=!state.reserved&&state.count<=NOVA_DESKTOP_LIMIT&&state.clipped<=1&&state.status<=4095;
   for(unsigned k=0;valid&&k<state.count;k++){unsigned len=0;while(len<64&&state.entries[k].name[len]){unsigned char c=(unsigned char)state.entries[k].name[len++];if(c<32||c==127||c=='/'||c=='\\')valid=0;}if(!len||len==64||(state.entries[k].kind!=1&&state.entries[k].kind!=2))valid=0;}
   if(!valid){nova_term_disconnect(71);return 1;}nova_copy(&g_desktop_files,&state,sizeof state);g_desktop_inflight=0;
   if(g_desktop_selected_name[0]){g_desktop_selected=~0U;for(unsigned k=0;k<state.count;k++)if(nova_streq(g_desktop_selected_name,state.entries[k].name)){g_desktop_selected=k;break;}if(g_desktop_selected==~0U)g_desktop_selected_name[0]=0;}
   changed=1;
  }
  else if(p.op==NOVA_TERM_DESKTOP_FILE_STATE){
   struct nova_desktop_file_reply reply;nova_copy(&reply,p.data,sizeof reply);
   int valid=g_desktop_file_inflight&&reply.action==g_desktop_file_inflight&&!reply.reserved&&reply.status<=4095&&reply.total<=NOVA_DESKTOP_TEXT_LIMIT&&reply.offset<=reply.total&&reply.length<=NOVA_DESKTOP_TEXT_BYTES&&reply.length<=reply.total-reply.offset;
   if(valid&&!reply.status&&reply.action==NOVA_DESKTOP_PREVIEW)valid=nova_desktop_basename_length(reply.name,64)&&nova_desktop_text_prefix(reply.text,reply.length,1)==(int)reply.length;
   if(!valid){nova_term_disconnect(71);return 1;}
   g_desktop_file_inflight=0;g_desktop_file_error=reply.status;g_desktop_file_last_action=reply.action;
   if(reply.action==NOVA_DESKTOP_PREVIEW)nova_copy(&g_desktop_preview,&reply,sizeof reply);
   else{g_desktop_pending=1;g_desktop_selected=~0U;if(g_desktop_trash_visible){nova_zero(&g_desktop_file_request,sizeof g_desktop_file_request);g_desktop_file_request.action=NOVA_DESKTOP_TRASH_LIST;g_desktop_file_pending=1;}}
   changed=1;
  }
  else if(p.op==NOVA_TERM_DESKTOP_TRASH_STATE){
   struct nova_desktop_trash_snapshot reply;nova_copy(&reply,p.data,sizeof reply);
   int valid=g_desktop_file_inflight==NOVA_DESKTOP_TRASH_LIST&&!reply.reserved&&reply.status<=4095&&reply.count<=NOVA_DESKTOP_TRASH_LIMIT&&reply.clipped<=1&&reply.total<=256&&reply.start<=reply.total&&reply.count<=reply.total-reply.start;
   for(unsigned k=0;valid&&k<reply.count;k++){struct nova_desktop_trash_entry*e=&reply.entries[k];valid=nova_desktop_basename_length(e->name,64)&&nova_desktop_token_valid(e->token)&&nova_streq(e->name,e->token+19)&&(e->kind==1||e->kind==2);}
   if(!valid){nova_term_disconnect(71);return 1;}
   nova_copy(&g_desktop_trash_files,&reply,sizeof reply);g_desktop_file_inflight=0;g_desktop_trash_selected=~0U;g_desktop_trash_scroll=0;changed=1;
  }
  else if(p.op==NOVA_TERM_DATA){nova_term_feed(&g_terminal.grid,p.data,p.length);changed=1;}
  else if(p.op==NOVA_TERM_READY){if(g_terminal.ready){nova_term_disconnect(71);return 1;}g_terminal.ready=1;g_terminal.persistent=(p.code&NOVA_TERM_PERSISTENT)!=0;g_terminal.resize_pending=1;if(!g_desktop_pending)g_desktop_pending=1;changed=1;}
  else if(p.op==NOVA_TERM_ERROR||p.op==NOVA_TERM_EXIT){if(p.op==NOVA_TERM_ERROR){nova_puts("[nova-terminal] session_error errno=");nova_put_u64(p.code);nova_putln("");}nova_term_disconnect(p.op==NOVA_TERM_ERROR?p.code:0);return 1;}
 }
 return changed;
}
static void nova_term_flush(void){
 if(!g_terminal.ready||g_terminal.fd<0)return;
 if(g_desktop_pending&&!g_desktop_inflight){long sent=nova_term_send(NOVA_TERM_DESKTOP,0,0,g_desktop_pending,0);if(sent>0){g_desktop_pending=0;g_desktop_inflight=1;}else if(sent!=-11&&sent!=-4)nova_term_disconnect((unsigned)-sent);}
 if(g_desktop_file_pending&&!g_desktop_file_inflight){long sent=nova_term_send(NOVA_TERM_DESKTOP_FILE,&g_desktop_file_request,sizeof g_desktop_file_request,0,0);if(sent>0){g_desktop_file_pending=0;g_desktop_file_inflight=g_desktop_file_request.action;}else if(sent!=-11&&sent!=-4)nova_term_disconnect((unsigned)-sent);}
 if(g_terminal.resize_pending){long n=nova_term_send(NOVA_TERM_RESIZE,0,0,g_terminal.rows,g_terminal.cols);if(n>0)g_terminal.resize_pending=0;else if(n!=-11&&n!=-4)nova_term_disconnect((unsigned)-n);}
 if(g_terminal.fd>=0&&g_terminal.queued){unsigned n=g_terminal.queued>NOVA_TERM_BYTES?NOVA_TERM_BYTES:g_terminal.queued;long sent=nova_term_send(NOVA_TERM_INPUT,g_terminal.input,n,0,0);if(sent>0){g_terminal.queued-=n;for(unsigned i=0;i<g_terminal.queued;i++)g_terminal.input[i]=g_terminal.input[i+n];g_terminal.backpressure=0;}else if(sent!=-11&&sent!=-4)nova_term_disconnect((unsigned)-sent);}
}
static int nova_term_input(unsigned key,unsigned shift,unsigned ctrl){
 if(key==104||key==109){if(key==104){g_terminal.grid.scroll+=g_terminal.grid.rows/2;if(g_terminal.grid.scroll>g_terminal.grid.history_count)g_terminal.grid.scroll=g_terminal.grid.history_count;}else g_terminal.grid.scroll=g_terminal.grid.scroll>g_terminal.grid.rows/2?g_terminal.grid.scroll-g_terminal.grid.rows/2:0;return 1;}
 const char *seq=key==103?"\033[A":key==108?"\033[B":key==105?"\033[D":key==106?"\033[C":key==102?"\033[H":key==107?"\033[F":key==111?"\033[3~":(const char *)0;
 if(seq){if(g_terminal.ready){unsigned n=(unsigned)nova_strlen(seq);if(n<=sizeof g_terminal.input-g_terminal.queued){for(unsigned i=0;i<n;i++)g_terminal.input[g_terminal.queued++]=(unsigned char)seq[i];g_terminal.grid.scroll=0;nova_term_flush();}else g_terminal.backpressure=1;}return 1;}
 unsigned c=nova_trq_key(key,shift,ctrl,g_terminal_altgr,g_terminal_caps);if(!c)return 0;
 unsigned char bytes[4];unsigned count=nova_keyboard_utf8(c,bytes);
 if(g_terminal.ready&&g_terminal.fd>=0){if(count<=sizeof g_terminal.input-g_terminal.queued){for(unsigned i=0;i<count;i++)g_terminal.input[g_terminal.queued++]=bytes[i];}else g_terminal.backpressure=1;g_terminal.grid.scroll=0;nova_term_flush();}
 return 1;
}
#endif
