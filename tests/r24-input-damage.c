#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#define nova_main unused_compositor
#include "../src/services/compositor.c"
#undef nova_main
int main(void){struct r24_hist hist={0};for(unsigned int i=1;i<=300;i++)r24_hist_add(&hist,i);assert(hist.n==256&&r24_percentile(&hist,50)==172&&r24_percentile(&hist,95)==288);
 unsigned int sizes[][2]={{640,400},{1280,800},{1600,900},{1920,1080}};
 for(unsigned int size=0;size<4;size++){unsigned int w=sizes[size][0],h=sizes[size][1],pitch=w*4+16;size_t bytes=(size_t)pitch*h;nova_zero(&g_scene,sizeof(g_scene));g_scene.configured=1;g_scene.panel_height=38;g_scene.visual_generation=1;g_r5_scale=1;g_r24_cursor_generation=0;g_r7_controls_visible=0;g_r7_library_visible=0;g_session_theme.choice=2;g_desktop_tasks.count=0;g_r2_canvas_width=w;g_r2_canvas_height=h;
 struct nova_scanout_buffer buf[2];memset(buf,0,sizeof(buf));unsigned char*raw[2],*reference=malloc(bytes+64);assert(reference);memset(reference,0x61,bytes+64);for(int k=0;k<2;k++){raw[k]=malloc(bytes+64);assert(raw[k]);memset(raw[k],0x61,bytes+64);buf[k].map=raw[k]+32;buf[k].pitch=pitch;}
 g_render.cursor_x=200;g_render.cursor_y=180;
 for(unsigned int frame=0;frame<32;frame++){int ox=g_render.cursor_x,oy=g_render.cursor_y;int plain=r24_plain_pointer(ox,oy,w,h);g_render.cursor_x=frame%8==0?(int)w-1:200+(int)frame%5;g_render.cursor_y=frame%8==0?(int)h-1:180+(int)frame%7;g_scene.visual_generation++;if(plain&&r24_plain_pointer(g_render.cursor_x,g_render.cursor_y,w,h))g_r24_cursor_generation++;
 if(frame==8){g_session_theme.choice=1;g_scene.visual_generation++;}if(frame==16){g_r7_controls_visible=2;g_scene.visual_generation++;}if(frame==24){g_r7_controls_visible=0;g_scene.visual_generation++;}
 (void)r24_render_scanout(&buf[frame%2],w,h,frame);r5_draw_scene(reference+32,w,h,pitch,frame);assert(!memcmp(buf[frame%2].map,reference+32,bytes));for(unsigned int j=0;j<32;j++)assert(raw[frame%2][j]==0x61&&raw[frame%2][bytes+32+j]==0x61);
 }
 g_r7_controls_visible=0;g_r7_library_visible=0;g_scene.dragging=0;g_client_grab.mode=0;g_session_theme.choice=2;
 int left=r7_dock_start(w),top=r7_dock_top(h),bx=r19_dock_slot_x(left,0);
 /* Alternate both scanouts: same-slot motion, slot edge, tooltip disappearance,
  * and overlay changes must remain byte-for-byte equal to a complete redraw. */
 for(unsigned int step=0;step<20;step++){
  int ox=g_render.cursor_x,oy=g_render.cursor_y;
  g_render.cursor_x=step<12?bx+10+(int)step:step<16?r19_dock_slot_x(left,1)+12+(int)(step-12):bx+15;
  g_render.cursor_y=step<18?top+25:top-5;
  int only=r24_pointer_motion_only(ox,oy,g_render.cursor_x,g_render.cursor_y,w,h);
  g_scene.visual_generation++;if(only)g_r24_cursor_generation++;
  if(step==6){g_session_theme.choice=1;g_scene.visual_generation++;}
  if(step==10){g_r7_controls_visible=2;g_scene.visual_generation++;}
  if(step==12){g_r7_controls_visible=0;g_scene.visual_generation++;}
  (void)r24_render_scanout(&buf[step%2],w,h,100+step);r5_draw_scene(reference+32,w,h,pitch,100+step);
  assert(!memcmp(buf[step%2].map,reference+32,bytes));
  for(unsigned int j=0;j<32;j++)assert(raw[step%2][j]==0x61&&raw[step%2][bytes+32+j]==0x61);
 }
 assert(r24_pointer_motion_only(bx+10,top+20,bx+12,top+22,w,h));
 assert(!r24_pointer_motion_only(bx+10,top+20,r19_dock_slot_x(left,1)+10,top+20,w,h));
 assert(!r24_pointer_motion_only(bx+10,top+20,bx+10,top-1,w,h));
 free(reference);free(raw[0]);free(raw[1]);}
 assert(g_r24.partial>0&&g_r24.full>0);g_r7_controls_visible=0;g_r7_library_visible=0;g_scene.dragging=0;g_client_grab.mode=0;g_r2_canvas_width=640;g_r2_canvas_height=400;g_render.cursor_x=200;g_render.cursor_y=180;g_input_modifiers=0;g_r17_alt_keys=0;
 int p[2];assert(!pipe(p));struct nova_input_event events[202];memset(events,0,sizeof(events));for(int i=0;i<192;i++){events[i].type=NOVA_EV_REL;events[i].code=NOVA_REL_X;events[i].value=1;}
 events[192]=(struct nova_input_event){0,0,NOVA_EV_KEY,NOVA_KEY_LEFTCTRL,1};events[193]=(struct nova_input_event){0,0,NOVA_EV_KEY,NOVA_BTN_LEFT,1};events[194]=(struct nova_input_event){0,0,NOVA_EV_KEY,NOVA_BTN_LEFT,0};events[195]=(struct nova_input_event){0,0,NOVA_EV_KEY,NOVA_KEY_LEFTCTRL,0};events[196]=(struct nova_input_event){0,0,NOVA_EV_SYN,3,0};events[197]=(struct nova_input_event){0,0,NOVA_EV_REL,NOVA_REL_X,777};events[198]=(struct nova_input_event){0,0,NOVA_EV_SYN,0,0};events[199]=(struct nova_input_event){0,0,NOVA_EV_REL,NOVA_REL_X,1};events[200]=(struct nova_input_event){0,0,NOVA_EV_KEY,NOVA_KEY_LEFTMETA,1};events[201]=(struct nova_input_event){0,0,NOVA_EV_KEY,NOVA_KEY_ESC,1};assert(write(p[1],events,sizeof(events))==sizeof(events));close(p[1]);unsigned long start=g_render.input_events;consume_input_fd(p[0],640,400);assert(g_render.input_events-start<=128&&g_r24.budget_hits);for(int i=0;i<5;i++)consume_input_fd(p[0],640,400);close(p[0]);assert(g_render.cursor_x==393&&!g_input_modifiers&&!g_r17_alt_keys&&!g_scene.dragging&&!g_r7_library_visible&&g_r24.queue_dropped==1);puts("R24_PIXEL_IDENTICAL_CURSOR_DAMAGE_4_SIZES_GUARDS_THEME_INVALIDATION_BOUNDED_INPUT_FINAL_POSITION_RELEASE_SYN_DROPPED=PASS");}
