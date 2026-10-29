#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#define nova_main unused_compositor
#include "../src/services/compositor.c"
#undef nova_main
static void key(unsigned int code){int p[2];assert(!pipe(p));struct nova_input_event e[2];memset(e,0,sizeof(e));e[0].type=e[1].type=NOVA_EV_KEY;e[0].code=e[1].code=code;e[0].value=1;e[1].value=0;assert(write(p[1],e,sizeof(e))==sizeof(e));close(p[1]);consume_input_fd(p[0],1600,900);close(p[0]);}
int main(void){
 g_scene.configured=1;g_scene.panel_height=38;g_r5_scale=1;g_r7_controls_visible=1;g_settings_page=2;g_r24_diagnostics_view=0;g_r2_canvas_width=1600;g_r2_canvas_height=900;
 r24_hist_add(&g_r24.draw,123);key(NOVA_KEY_RIGHT);assert(g_r24_diagnostics_view==1);key(NOVA_KEY_ENTER);assert(g_r24.draw.n==1);key(NOVA_KEY_LEFT);assert(g_r24_diagnostics_view==0);key(NOVA_KEY_ENTER);assert(g_r24.draw.n==0);
 int x,y;unsigned int pw,ph;r21_settings_layout(1600,900,&x,&y,&pw,&ph);struct nova_ui_rect tab=nova_ui_segment_rect(x+154,y+80,(int)pw-176,28,2,6,1);assert(r7_chrome_click(tab.x+tab.w/2,tab.y+14));assert(g_r24_diagnostics_view==1);tab=nova_ui_segment_rect(x+154,y+80,(int)pw-176,28,2,6,0);assert(r7_chrome_click(tab.x+10,tab.y+14));assert(g_r24_diagnostics_view==0);
 unsigned int sizes[][2]={{400,400},{640,400},{1280,800},{1600,900},{1920,1080}};
 for(unsigned int i=0;i<5;i++)for(unsigned int theme=0;theme<3;theme++)for(unsigned int view=0;view<2;view++){
  unsigned int w=sizes[i][0],h=sizes[i][1],pitch=w*4U+16U;size_t bytes=(size_t)pitch*h;unsigned char*raw=malloc(bytes+64);assert(raw);memset(raw,0x61,bytes+64);g_session_theme.choice=theme;g_r24_diagnostics_view=view;g_render.theme=0;draw_scene(raw+32,w,h,pitch,0);
  int sx,sy;unsigned int sw,sh;r21_settings_layout(w,h,&sx,&sy,&sw,&sh);assert(sy+(int)sh<=r7_dock_top(h)-12);assert(sx>=0&&sx+(int)sw<=(int)w);
  struct nova_ui_rect active=nova_ui_segment_rect(sx+154,sy+r24_diagnostics_tab_y(sh),(int)sw-176,28,2,6,view);assert(r7_chrome_click(active.x+active.w/2,active.y+14));assert(g_r24_diagnostics_view==view);
  for(unsigned int k=0;k<32;k++)assert(raw[k]==0x61&&raw[bytes+32+k]==0x61);
  for(unsigned int yy=0;yy<h;yy++)for(unsigned int k=w*4U;k<pitch;k++)assert(raw[32+(size_t)yy*pitch+k]==0x61);
  const char*out=getenv("NOVA_R24_EVIDENCE_DIR");if(out&&theme==2&&(w==640||w==1600)){char path[4096];snprintf(path,sizeof(path),"%s/%ux%u-graphics-%s-HOST-NOT-VM.ppm",out,w,h,view?"renderer":"timings");FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(unsigned int yy=0;yy<h;yy++){unsigned int*row=(void*)(raw+32+(size_t)yy*pitch);for(unsigned int xx=0;xx<w;xx++){unsigned char rgb[3]={(unsigned char)(row[xx]>>16),(unsigned char)(row[xx]>>8),(unsigned char)row[xx]};assert(fwrite(rgb,1,3,f)==3);}}assert(!fclose(f));}
  free(raw);
 }
 key(NOVA_KEY_ESC);assert(!g_r7_controls_visible);puts("R24_RENDERER_TABS_REAL_KEY_POINTER_NO_HIDDEN_RESET_30_RENDER_PADDING_GUARDS=PASS");return 0;
}
