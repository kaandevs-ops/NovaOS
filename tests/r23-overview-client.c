#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static void key(unsigned int code,int value){int p[2];assert(!pipe(p));struct nova_input_event e={0};e.type=NOVA_EV_KEY;e.code=code;e.value=value;assert(write(p[1],&e,sizeof(e))==sizeof(e));close(p[1]);consume_input_fd(p[0],1600,900);close(p[0]);}
int main(void){struct nova_overview_reply q={NOVA_OVERVIEW_MAGIC,1,sizeof(q),7,3,2,1,1,0,0,0,0,0,0,100};assert(nova_overview_reply_valid(&q,7));assert(!nova_overview_reply_valid(&q,8));q.apps_running=4;assert(!nova_overview_reply_valid(&q,7));q.apps_running=1;
 int pair[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0,pair));g_overview_fd=pair[0];g_overview_sequence=7;g_overview_pending=1;g_overview_next=~0ULL;g_overview_deadline=1000;g_r7_controls_visible=1;g_settings_page=1;
 assert(send(pair[1],&q,sizeof(q),0)==sizeof(q));r23_overview_tick(1);assert(g_overview_ready&&!g_overview_pending);assert(!r23_overview_accept(&q));
 g_overview_pending=1;q.reserved=1;assert(send(pair[1],&q,sizeof(q),0)==sizeof(q));r23_overview_tick(2);assert(g_overview_fd<0&&!g_overview_ready);close(pair[1]);
 assert(!socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK,0,pair));g_overview_fd=pair[0];g_overview_pending=1;g_overview_deadline=3;r23_overview_tick(4);assert(g_overview_fd<0);close(pair[1]);
 unsigned int sizes[][2]={{400,400},{640,480},{1280,800},{1600,900},{1920,1080}};
 for(unsigned int j=0;j<5;j++)for(unsigned int t=0;t<3;t++){unsigned int w=sizes[j][0],h=sizes[j][1];unsigned long bytes=(unsigned long)w*h*4;unsigned char*b=malloc(bytes+64);assert(b);memset(b,0x62,bytes+64);g_r7_controls_visible=1;g_settings_page=1;g_session_theme.choice=t;r7_controls(b+32,w,h,w*4);for(unsigned int k=0;k<32;k++)assert(b[k]==0x62&&b[bytes+32+k]==0x62);free(b);}
 for(unsigned int i=0;i<10000;i++)r23_overview_tick(10);assert(g_overview_fd<0&&!g_overview_pending);g_overview_last_request=100;g_overview_next=0;r23_overview_tick(101);assert(g_overview_next==1000000100ULL&&g_overview_fd<0);g_settings_page=0;r23_overview_tick(11);assert(!g_overview_ready);
 g_scene.panel_height=38;g_r2_canvas_width=1600;g_r2_canvas_height=900;g_r7_controls_visible=2;g_settings_focus=4;key(NOVA_KEY_ENTER,1);assert(g_r7_controls_visible==1&&g_settings_page==1);key(59,1);assert(g_settings_page==0);key(59,1);assert(g_settings_page==1);key(NOVA_KEY_ESC,1);assert(!g_r7_controls_visible);key(NOVA_KEY_LEFTCTRL,1);key(56,1);key(23,1);assert(g_r7_controls_visible==1&&g_settings_page==1);g_input_modifiers=0;g_r17_alt_keys=0;key(NOVA_KEY_ESC,1);
 puts("R23_OVERVIEW_BOUNDED_REPLY_REPLAY_INVALID_COUNTS_TIMEOUT_DISCONNECT_RENDER_QUIET_REFRESH=PASS");return 0;}
