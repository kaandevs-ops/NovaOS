#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
int main(void){
 g_desktop.connected=1;g_scene.panel_height=NOVA_UI_PANEL_HEIGHT;
 nova_zero(&g_desktop_tasks,sizeof(g_desktop_tasks));g_desktop_tasks.count=3;
 for(unsigned int i=0;i<3;i++){g_desktop_tasks.entries[i].client_id=i+1;g_desktop_tasks.entries[i].flags=NOVA_DESKTOP_TASK_FLAG_LAUNCHABLE;strcpy(g_desktop_tasks.entries[i].display_name,"Native App");}
 nova_zero(g_surfaces,sizeof(g_surfaces));g_surfaces[0].used=1;g_surfaces[0].committed=1;g_surfaces[0].client_id=3;g_surfaces[0].surface_id=300;
 const unsigned int widths[]={1280,1600,1920};
 for(unsigned int n=0;n<3;n++){
  g_r2_canvas_width=widths[n];g_r2_canvas_height=n==0?800:n==1?900:1080;
  int left=r7_dock_start(g_r2_canvas_width),top=r7_dock_top(g_r2_canvas_height);
  for(unsigned int i=0;i<2;i++)assert(r7_dock_system_hit(r19_dock_slot_x(left,i)+28,top+30)==(int)i+1);
  for(unsigned int i=0;i<3;i++){
   unsigned int id,action;int bx=r19_dock_slot_x(left,i+2);
   assert(nova_r2_dock_hit(bx+28,top+30,&id,&action)&&id==i+1&&action==(i==2?NOVA_DESKTOP_ACTION_ACTIVATE:NOVA_DESKTOP_ACTION_LAUNCH));
   assert(!nova_r2_dock_hit(bx+56,top+30,&id,&action));
   int mx=NOVA_UI_TASK_BASE+(int)i*NOVA_UI_TASK_PITCH;
   assert(desktop_task_hit(mx+20,20,&id,&action)&&id==i+1&&action==(i==2?NOVA_DESKTOP_ACTION_ACTIVATE:NOVA_DESKTOP_ACTION_LAUNCH));
   assert(desktop_task_hit(mx+90,20,&id,&action)&&id==i+1&&action==(i==2?NOVA_DESKTOP_ACTION_CLOSE:NOVA_DESKTOP_ACTION_LAUNCH));
   assert(!desktop_task_hit(mx+20,33,&id,&action));
  }
  unsigned int w=g_r2_canvas_width,h=g_r2_canvas_height;unsigned long bytes=(unsigned long)w*h*4;unsigned char*p=malloc(bytes+64);assert(p);memset(p,0x57,bytes+64);
  g_r2_canvas_width=w;g_r2_canvas_height=h;g_render.cursor_x=left+25;g_render.cursor_y=top+30;
  r3_panel(p,w,h,w*4,g_scene.panel_height);draw_desktop_taskbar(p,w,h,w*4);nova_r2_dock(p,w,h,w*4);
  for(unsigned int k=0;k<64;k++)assert(p[bytes+k]==0x57);free(p);
 }
 /* Reserved Meta opens/closes the real application menu without forwarding to apps. */
 int meta_pipe[2];assert(!pipe(meta_pipe));struct nova_input_event meta={0};meta.type=NOVA_EV_KEY;meta.code=NOVA_KEY_LEFTMETA;meta.value=1;
 assert(write(meta_pipe[1],&meta,sizeof(meta))==sizeof(meta));close(meta_pipe[1]);consume_input_fd(meta_pipe[0],1600,900);close(meta_pipe[0]);assert(g_r7_library_visible);assert(r19_library_key(NOVA_KEY_END)&&g_r19_menu_selected==2);assert(r19_library_key(NOVA_KEY_HOME)&&!g_r19_menu_selected);assert(r19_library_key(NOVA_KEY_ESC));
 int pair[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET,0,pair));g_desktop_fd=pair[0];g_r2_canvas_width=1600;g_r2_canvas_height=900;
 int left=r7_dock_start(1600),top=r7_dock_top(900);assert(r7_chrome_click(r19_dock_slot_x(left,0)+28,top+30)&&g_r7_library_visible);
 int mx,my;unsigned int mw,mh;r19_library_layout(1600,900,&mx,&my,&mw,&mh);assert(r19_overlay_at(mx+30,my+80)&&!r19_overlay_at(10,200));
 int input[2];assert(!pipe(input));struct nova_input_event events[2];nova_zero(events,sizeof(events));events[0].type=events[1].type=NOVA_EV_KEY;events[0].code=NOVA_KEY_DOWN;events[1].code=NOVA_KEY_ENTER;events[0].value=events[1].value=1;
 assert(write(input[1],events,sizeof(events))==(long)sizeof(events));close(input[1]);consume_input_fd(input[0],1600,900);close(input[0]);
 struct nova_desktop_message event;assert(recv(pair[1],&event,sizeof(event),0)==sizeof(event));assert(event.opcode==NOVA_DESKTOP_OP_TASK_EVENT&&event.client_id==2&&event.action==NOVA_DESKTOP_ACTION_LAUNCH&&!g_r7_library_visible);
 close(pair[0]);close(pair[1]);g_desktop_fd=-1;
 g_r2_canvas_width=400;g_r2_canvas_height=800;unsigned int id,action;assert(!desktop_task_hit(280,20,&id,&action));
 puts("R19_ACTUAL_DOCK_MENU_GEOMETRY_HIT_GAPS_VISIBLE_CLIPPING_LAUNCH_ACTIVATE_CLOSE=PASS");
 puts("R19_MENU_DOCK_THREE_RESOLUTIONS_PIXEL_GUARDS=PASS");return 0;
}
