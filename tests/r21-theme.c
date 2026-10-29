#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static void input_key(unsigned int key){int p[2];assert(!pipe(p));struct nova_input_event ev={0};ev.type=NOVA_EV_KEY;ev.code=key;ev.value=1;assert(write(p[1],&ev,sizeof(ev))==sizeof(ev));close(p[1]);consume_input_fd(p[0],1600,900);close(p[0]);}
int main(void){
 assert(nova_ui_segment_hit(10,20,300,72,3,6,20,30)==0);assert(nova_ui_segment_hit(10,20,300,72,3,6,108,30)==-1);assert(!nova_ui_segment_rect(0,0,10,10,0,6,0).w);
 int pair[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0,pair));struct nova_client_connection*c=&g_connections[0];nova_zero(c,sizeof(*c));c->used=1;c->fd=pair[0];c->authenticated=1;c->client_id=3;c->theme_subscribed=1;c->effective_capabilities=NOVA_APP_CAP_UI_SURFACE;
 struct nova_client_surface*s=&g_surfaces[0];nova_zero(s,sizeof(*s));s->used=1;s->owner_fd=pair[0];s->client_id=3;s->surface_id=300;
 r21_flush_theme(c);struct nova_surface_event e;assert(recv(pair[1],&e,sizeof(e),0)==sizeof(e));struct nova_theme_state state={2,0};assert(nova_theme_accept(&state,&e,3,300,0));assert(!nova_theme_accept(&state,&e,3,300,0));
 struct nova_surface_event bad=e;bad.value=2;bad.code=3;assert(!nova_theme_accept(&state,&bad,3,300,0));bad=e;bad.value=2;bad.client_id=1;assert(!nova_theme_accept(&state,&bad,3,300,0));bad=e;bad.value=2;bad.surface_id=400;assert(!nova_theme_accept(&state,&bad,3,300,0));bad=e;bad.value=-1;assert(!nova_theme_accept(&state,&bad,3,300,0));bad=e;bad.value=2;bad.width=1;assert(!nova_theme_accept(&state,&bad,3,300,0));
 assert(!r21_set_theme(3)&&g_session_theme.choice==2);assert(r21_set_theme(1));assert(recv(pair[1],&e,sizeof(e),0)==sizeof(e)&&nova_theme_accept(&state,&e,3,300,0)&&state.choice==1);
 c->theme_subscribed=0;assert(r21_set_theme(0));assert(recv(pair[1],&e,sizeof(e),MSG_DONTWAIT)==-1&&errno==EAGAIN);
 struct nova_surface_message m;nova_zero(&m,sizeof(m));m.magic=NOVA_SURFACE_PROTOCOL_MAGIC;m.version=6;m.size=sizeof(m);m.client_id=3;m.surface_id=300;m.opcode=NOVA_SURFACE_EVENT_THEME;m.input_flags=1;
 assert(send(pair[1],&m,sizeof(m),0)==sizeof(m)&&consume_client_message(pair[0],1600,900));assert(g_session_theme.choice==0);/* no client SET route */
 m.opcode=NOVA_SURFACE_OP_HELLO;m.surface_id=0;m.input_flags=NOVA_SURFACE_FEATURE_THEME;assert(send(pair[1],&m,sizeof(m),0)==sizeof(m)&&consume_client_message(pair[0],1600,900));assert(c->theme_subscribed);assert(recv(pair[1],&e,sizeof(e),0)==sizeof(e));
 assert(fcntl(pair[0],F_SETFL,fcntl(pair[0],F_GETFL)&~O_NONBLOCK)==0);int small=4096;assert(!setsockopt(pair[0],SOL_SOCKET,SO_SNDBUF,&small,sizeof(small)));char fill[136]={0};while(send(pair[0],fill,sizeof(fill),MSG_NOSIGNAL|MSG_DONTWAIT)>0){}assert(errno==EAGAIN);unsigned int old=c->theme_sent;assert(r21_set_theme(1)&&r21_set_theme(2));assert(c->theme_sent==old);
 struct nova_pollfd fds[3];int clients[3]={pair[0],-1,-1};setup_app_pollfds(fds,0,clients);assert(fds[0].events&4);while(recv(pair[1],fill,sizeof(fill),MSG_DONTWAIT)>0){}r21_flush_theme(c);assert(recv(pair[1],&e,sizeof(e),0)==sizeof(e)&&e.code==2&&e.value==(int)g_session_theme.generation);r21_flush_theme(c);assert(recv(pair[1],&e,sizeof(e),MSG_DONTWAIT)==-1&&errno==EAGAIN);
 close(pair[1]);assert(r21_set_theme(0));/* MSG_NOSIGNAL: closed subscriber cannot kill compositor */close(pair[0]);nova_zero(c,sizeof(*c));nova_zero(s,sizeof(*s));
 g_r2_canvas_width=1600;g_r2_canvas_height=900;
 input_key(NOVA_KEY_LEFTCTRL);input_key(56);input_key(NOVA_KEY_S);assert(g_r7_controls_visible);g_input_modifiers=0;g_r17_alt_keys=0;input_key(NOVA_KEY_ESC);
int left=r7_dock_start(1600),top=r7_dock_top(900);assert(r7_chrome_click(r19_dock_slot_x(left,1)+28,top+20)&&g_r7_controls_visible);input_key(NOVA_KEY_RIGHT);input_key(NOVA_KEY_ENTER);assert(g_session_theme.choice==1);input_key(NOVA_KEY_ESC);assert(!g_r7_controls_visible);
 unsigned int widths[]={400,640,1280,1600,1920},heights[]={400,480,800,900,1080};
 for(unsigned int i=0;i<5;i++)for(unsigned int theme=0;theme<3;theme++){g_session_theme.choice=theme;g_r7_controls_visible=1;unsigned int w=widths[i],h=heights[i];unsigned long bytes=(unsigned long)w*h*4;unsigned char*b=malloc(bytes+64);assert(b);memset(b,0x71,bytes+64);r7_controls(b,w,h,w*4);for(unsigned int k=0;k<64;k++)assert(b[bytes+k]==0x71);free(b);}
 puts("R21_THEME_OPTIN_AUTH_BOUND_EVENT_REPLAY_SPOOF_BACKPRESSURE_CLOSED_PEER_SETTINGS_KEYS_BOUNDS=PASS");return 0;
}
