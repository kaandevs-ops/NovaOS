#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static void drain(int fd){char bytes[512];while(recv(fd,bytes,sizeof(bytes),MSG_DONTWAIT)>0){}}
static void evdev_key(unsigned int code,int value){
 int p[2];assert(pipe(p)==0);struct nova_input_event ev;nova_zero(&ev,sizeof(ev));ev.type=NOVA_EV_KEY;ev.code=(unsigned short)code;ev.value=value;
 assert(write(p[1],&ev,sizeof(ev))==(long)sizeof(ev));close(p[1]);consume_input_fd(p[0],1280,800);close(p[0]);
}
int main(void){
 int pair[2];assert(socketpair(AF_UNIX,SOCK_SEQPACKET,0,pair)==0);
 g_scene.configured=1;g_scene.panel_height=58;g_r2_canvas_width=1280;g_r2_canvas_height=800;
 g_connections[0].used=1;g_connections[0].authenticated=1;g_connections[0].client_id=3;g_connections[0].fd=pair[0];
 g_connections[0].effective_capabilities=NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT;
 for(unsigned int i=0;i<2;i++){
  struct nova_client_surface*s=&g_surfaces[i];s->used=s->committed=1;s->owner_fd=pair[0];s->client_id=3;s->surface_id=300+i;s->role=NOVA_SURFACE_ROLE_TOPLEVEL;
  s->x=s->pending_x=80+(int)i*30;s->y=s->pending_y=100+(int)i*30;s->width=s->pending_width=800;s->height=s->pending_height=500;
  s->min_width=800;s->min_height=500;s->max_width=1280;s->max_height=800;s->z_index=i+1;s->buffer_id=1;
 }
 struct nova_client_surface*s=&g_surfaces[0];set_client_focus(300);drain(pair[1]);
 assert(top_client_surface_at(200,200)==300);unsigned int focus=g_client.focus_surface;
 set_client_focus(123456);assert(g_client.focus_surface==focus);
 for(int button=1;button<=3;button++){
  int x=s->x+(int)s->width-6-button*24+7;
  assert(r17_title_button_at(s,x,s->y+12)==button);
 }
 evdev_key(56,1);evdev_key(NOVA_KEY_M,1);evdev_key(56,0);assert(s->minimized&&g_client.focus_surface!=300);
 assert(!route_client_event(NOVA_SURFACE_EVENT_POINTER,300,NOVA_BTN_LEFT,1));
 evdev_key(56,1);evdev_key(NOVA_KEY_TAB,1);evdev_key(56,0);assert(g_client.focus_surface==300&&!s->minimized);drain(pair[1]);
 evdev_key(56,1);evdev_key(68,1);evdev_key(56,0);assert(s->maximized&&s->awaiting_configure&&s->configure_width==1264&&s->configure_height==626);
 assert(!r17_toggle_maximize(s,1280,800));s->awaiting_configure=0;
 assert(r17_toggle_maximize(s,1280,800)&&s->configure_width==800&&s->configure_height==500);s->awaiting_configure=0;drain(pair[1]);
 s->pending_x=-1000;s->pending_y=-1000;clamp_client_surface(s,1280,800);assert(s->pending_x==0&&s->pending_y==58);
 s->pending_x=99999;s->pending_y=99999;clamp_client_surface(s,1280,800);assert(s->pending_x==480&&s->pending_y==300);
 evdev_key(56,1);evdev_key(62,1);evdev_key(56,0);struct nova_surface_event event;
 assert(read(pair[1],&event,sizeof(event))==(long)sizeof(event)&&event.type==NOVA_SURFACE_EVENT_CLOSE_REQUEST&&event.client_id==3);
 drain(pair[1]);
 struct nova_render_metrics metrics={0};
 for(unsigned int i=0;i<=60;i++)nova_present_sample(&metrics,1000000000ULL+(unsigned long long)i*1000000000ULL/60U,0);
 assert(metrics.presents==61&&metrics.fps_milli==60000&&metrics.fps_window_end_ns==2000000000ULL);
 assert(!nova_render_needed(0,7,7)&&!nova_render_needed(1,7,8)&&nova_render_needed(0,7,8));
 unsigned int idle_draws=0;for(unsigned int i=0;i<10000;i++)if(nova_render_needed(0,7,7))idle_draws++;
 assert(!idle_draws);nova_render_sample(&metrics,100,70);assert(metrics.draw_ns==100&&metrics.draw_cpu_ns==70);
 nova_present_sample(&metrics,3000000000ULL,2990000000ULL);assert(metrics.input_to_present_ns==10000000ULL);
 r17_sample_resources();assert(g_r17_rss_available&&g_r17_rss_bytes>0);
 assert(r17_round_coverage(0,0,800,500)==0&&r17_round_coverage(100,100,800,500)==255);
 /* Integer 2x is tested for pixel coverage only, not claimed as native HiDPI fonts. */
 nova_zero(g_surfaces,sizeof(g_surfaces));g_r5_scale=2;g_r5_logical_w=800;g_r5_logical_h=500;
 unsigned long size=1600UL*1000UL*4UL;unsigned char*pixels=malloc(size+32);assert(pixels);
 for(unsigned long i=0;i<size+32;i++)pixels[i]=0xa7;
 r5_draw_scene(pixels,1600,1000,6400,0);
 for(unsigned int y=0;y<1000;y+=137)for(unsigned int x=0;x<1600;x+=137){
  unsigned int yy=y&~1U,xx=x&~1U;unsigned int*a=(unsigned int*)(pixels+(unsigned long)yy*6400)+xx;
  assert(a[0]==a[1]&&a[0]==a[1600]&&a[0]==a[1601]);
 }
 for(unsigned int i=0;i<32;i++)assert(pixels[size+i]==0xa7);free(pixels);close(pair[0]);close(pair[1]);
 puts("R17_REAL_EVDEV_SHORTCUTS_WM_HIT_TEST_FOCUS_AND_BOUNDS=PASS");
 puts("R17_RENDER_IDLE_GATE_METRICS_INJECTED_CLOCK_RSS_2X_PIXEL_GUARDS=PASS (synthetic clock is not actual display FPS)");return 0;
}
