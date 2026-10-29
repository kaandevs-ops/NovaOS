#ifndef NOVA_SOFTWARE_CLIENT_H
#define NOVA_SOFTWARE_CLIENT_H
#include "nova_software_protocol.h"
#include "nova_terminal_grid.h"
struct nova_sw_client{int fd;long child;unsigned started,ready,error,tag,pending,pending_tag,last_reply,visible,focused,page,selected,tab,scroll,confirm,drag,maximized;int x,y,w,h,gx,gy;unsigned needs_list,close_requested,input_turn,queued[4],pending_bytes,pending_index;unsigned char input[4][4096];unsigned running,exit_code[4],installed[4],last_result,last_action,app_cols[4],app_rows[4];struct nova_term_grid grids[4];};
static struct nova_sw_client g_sw={.fd=-1,.installed={4,4,4,4}};
static long nova_sw_send(unsigned op,unsigned index,const void*data,unsigned length){
 if(!g_sw.ready||g_sw.fd<0||length>NOVA_SW_BYTES)return -1;
 struct nova_sw_packet p;nova_zero(&p,sizeof p);p.magic=NOVA_SW_MAGIC;p.version=1;p.op=op;p.index=index;p.tag=g_sw.tag+1;p.length=length;if(length)nova_copy(p.data,data,length);
 if(!nova_sw_valid(&p,32UL+length,1))return -1;
 struct nova_iovec io={&p,32UL+length};struct nova_msghdr m;nova_zero(&m,sizeof m);m.msg_iov=&io;m.msg_iovlen=1;
 long n=nova_sendmsg(g_sw.fd,&m,NOVA_MSG_DONTWAIT|NOVA_MSG_NOSIGNAL);if(n==(long)(32+length))g_sw.tag=p.tag;return n;
}
static void nova_sw_down(unsigned code){if(g_sw.fd>=0)nova_close(g_sw.fd);g_sw.fd=-1;g_sw.ready=0;g_sw.pending=0;g_sw.running=0;g_sw.error=code?code:5;g_sw.pending_bytes=0;g_sw.close_requested=0;for(unsigned i=0;i<4;i++)g_sw.queued[i]=0;}
static void nova_sw_reap(void){if(g_sw.child>0){int st;long got=nova_wait4(g_sw.child,&st,NOVA_WNOHANG);if(got==g_sw.child||got==-10)g_sw.child=0;}}
static int nova_sw_start(void){
 nova_sw_reap();if(g_sw.fd>=0)return 1;if(g_sw.child>0)return 0;g_sw.started=1;
 int fds[2];long rc=nova_syscall4(NOVA_TERM_NR_SOCKETPAIR,NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK|0x80000,0,(long)fds);if(rc<0){g_sw.error=(unsigned)-rc;return 0;}
 long child=nova_clone_process();if(child<0){nova_close(fds[0]);nova_close(fds[1]);g_sw.error=(unsigned)-child;return 0;}
 if(!child){nova_close(fds[0]);if(fds[1]!=3){if(nova_syscall3(NOVA_TERM_NR_DUP3,fds[1],3,0)<0)nova_exit(126);}else if(nova_syscall3(NOVA_TERM_NR_FCNTL,3,2,0)<0)nova_exit(126);if(nova_close_range(4,~0U,0)<0)nova_exit(126);for(int i=0;i<3;i++)nova_close(i);char*args[]={NOVA_SOFTWARE_HELPER,0};char*env[]={0};nova_execve(args[0],args,env);nova_exit(127);}
 nova_close(fds[1]);g_sw.fd=fds[0];g_sw.child=child;g_sw.ready=0;g_sw.error=0;g_sw.pending=0;g_sw.tag=0;g_sw.last_reply=0;g_sw.running=0;g_sw.needs_list=1;g_sw.close_requested=0;g_sw.pending_bytes=0;
 for(unsigned i=0;i<4;i++){g_sw.queued[i]=0;g_sw.installed[i]=4;nova_term_grid_init(&g_sw.grids[i],24,80);g_sw.app_rows[i]=24;g_sw.app_cols[i]=80;}return 1;
}
static int nova_sw_request_data(unsigned action,unsigned app,const void*data,unsigned length){if(g_sw.pending)return 0;long n=nova_sw_send(action,app,data,length);if(n>0){g_sw.pending=action;g_sw.pending_tag=g_sw.tag;g_sw.pending_index=app;g_sw.last_result=0;return 1;}return 0;}
static int nova_sw_request(unsigned action,unsigned app){return nova_sw_request_data(action,app,0,0);}
static int nova_sw_input(unsigned app,const void*data,unsigned length){
 if(app>=4||!length||length>sizeof g_sw.input[app]-g_sw.queued[app]){g_sw.last_result=105;return 0;}
 nova_copy(g_sw.input[app]+g_sw.queued[app],data,length);g_sw.queued[app]+=length;return 1;
}
static void nova_sw_flush(void){
 if(!g_sw.ready||g_sw.pending)return;
 for(unsigned i=0;i<4;i++)if(g_sw.close_requested&(1U<<i)){g_sw.queued[i]=0;if(nova_sw_request(NOVA_SW_CLOSE,i))g_sw.close_requested&=~(1U<<i);return;}
 if(g_sw.needs_list){if(nova_sw_request(NOVA_SW_LIST,0))g_sw.needs_list=0;return;}
 for(unsigned j=0;j<4;j++){unsigned i=(g_sw.input_turn+j)%4;if(!g_sw.queued[i])continue;if(!(g_sw.running&(1U<<i))){g_sw.queued[i]=0;continue;}
 unsigned n=g_sw.queued[i]>128?128:g_sw.queued[i];if(nova_sw_request_data(NOVA_SW_INPUT,i,g_sw.input[i],n)){g_sw.pending_bytes=n;g_sw.pending_index=i;g_sw.input_turn=(i+1)%4;}return;}
}
static int nova_sw_receive(void){
 int changed=0;for(unsigned turn=0;turn<4;turn++){
  struct nova_sw_packet p;struct nova_iovec io={&p,sizeof p};struct nova_msghdr msg;nova_zero(&msg,sizeof msg);msg.msg_iov=&io;msg.msg_iovlen=1;long n=nova_recvmsg(g_sw.fd,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_TRUNC);
  if(n==-11||n==-4)break;if(n<=0){nova_sw_down(n<0?(unsigned)-n:5);return 1;}if(msg.msg_flags||!nova_sw_valid(&p,(unsigned long)n,0)){nova_sw_down(71);return 1;}
  if(p.op==NOVA_SW_READY){if(g_sw.ready){nova_sw_down(71);return 1;}g_sw.ready=1;g_sw.needs_list=1;nova_sw_flush();changed=1;}
  else if(!g_sw.ready){nova_sw_down(71);return 1;}
  else if(p.op==NOVA_SW_STATE){struct nova_sw_state s;nova_copy(&s,p.data,sizeof s);if(p.tag>g_sw.tag||p.tag<=g_sw.last_reply||(p.tag!=g_sw.pending_tag||s.action!=g_sw.pending||p.index!=g_sw.pending_index||s.app!=g_sw.pending_index)){nova_sw_down(71);return 1;}g_sw.last_reply=p.tag;
   if(p.tag==g_sw.pending_tag){
    if(g_sw.pending==NOVA_SW_INPUT&&g_sw.pending_bytes){unsigned i=g_sw.pending_index,n=g_sw.pending_bytes;if(!s.result&&n<=g_sw.queued[i]){for(unsigned k=n;k<g_sw.queued[i];k++)g_sw.input[i][k-n]=g_sw.input[i][k];g_sw.queued[i]-=n;}else if(s.result!=16)g_sw.queued[i]=0;g_sw.pending_bytes=0;}
    g_sw.pending=0;g_sw.pending_tag=0;}g_sw.running=s.running;g_sw.last_result=s.result;g_sw.last_action=s.action;for(unsigned i=0;i<4;i++)g_sw.installed[i]=s.installed[i];changed=1;
  }else if(p.op==NOVA_SW_DATA){nova_term_feed(&g_sw.grids[p.index],p.data,p.length);if(g_sw.visible&&g_sw.page==p.index+1)changed=1;}
  else if(p.op==NOVA_SW_EXIT){g_sw.running&=~(1U<<p.index);g_sw.queued[p.index]=0;g_sw.exit_code[p.index]=p.code;changed=1;}
 }return changed;
}
#endif
