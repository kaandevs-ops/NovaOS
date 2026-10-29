#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static void key(unsigned int code,int value){int p[2];assert(!pipe(p));struct nova_input_event e={0};e.type=NOVA_EV_KEY;e.code=code;e.value=value;assert(write(p[1],&e,sizeof(e))==sizeof(e));close(p[1]);consume_input_fd(p[0],1600,900);close(p[0]);}
int main(void){
 g_scene.panel_height=38;g_r2_canvas_width=1600;g_r2_canvas_height=900;g_r7_controls_visible=0;assert(r7_chrome_click(1550,20)&&g_r7_controls_visible==2);
 int x,y;unsigned int w,h;r22_quick_layout(1600,900,&x,&y,&w,&h);assert(x>=0&&y>=38&&x+(int)w<=1600&&y+(int)h<=900);assert(r19_overlay_at(x+20,y+80));
 unsigned int gen=g_session_theme.generation;assert(r7_chrome_click(x+20,y+20)&&g_session_theme.generation==gen);
 struct nova_ui_rect q=nova_ui_segment_rect(x+16,y+76,(int)w-32,74,3,6,1);assert(r7_chrome_click(q.x+10,q.y+10)&&g_session_theme.choice==1&&g_quick_pressed==1);key(NOVA_BTN_LEFT,0);assert(g_quick_pressed==~0U);
 assert(!r7_chrome_click(20,100)&&!g_r7_controls_visible);assert(r7_chrome_click(1550,20)&&g_settings_focus==1);key(NOVA_KEY_RIGHT,1);key(NOVA_KEY_ENTER,1);assert(g_session_theme.choice==2);key(NOVA_KEY_TAB,1);assert(g_settings_focus==3);key(NOVA_KEY_ENTER,1);assert(g_r7_controls_visible==1);key(NOVA_KEY_ESC,1);assert(!g_r7_controls_visible);
 key(NOVA_KEY_LEFTCTRL,1);key(56,1);key(NOVA_KEY_Q,1);assert(g_r7_controls_visible==2);key(NOVA_KEY_Q,1);assert(!g_r7_controls_visible);g_input_modifiers=0;g_r17_alt_keys=0;
 unsigned int sizes[][2]={{400,400},{640,480},{1280,800},{1600,900},{1920,1080}};
 for(unsigned int i=0;i<5;i++)for(unsigned int t=0;t<3;t++){unsigned int ww=sizes[i][0],hh=sizes[i][1];unsigned long bytes=(unsigned long)ww*hh*4;unsigned char*b=malloc(bytes+64);assert(b);memset(b,0x53,bytes+64);g_r7_controls_visible=2;g_session_theme.choice=t;r7_controls(b+32,ww,hh,ww*4);for(unsigned int k=0;k<32;k++)assert(b[k]==0x53&&b[bytes+32+k]==0x53);free(b);}
 puts("R22_QUICK_REAL_MOUSE_KEYS_CENTRAL_THEME_REOPEN_OUTSIDE_OVERLAY_PRESSED_BOUNDS=PASS");return 0;
}
