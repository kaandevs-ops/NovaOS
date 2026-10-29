#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static void key(unsigned code){int p[2];assert(!pipe(p));struct nova_input_event e={0};e.type=NOVA_EV_KEY;e.code=code;e.value=1;assert(write(p[1],&e,sizeof e)==sizeof e);close(p[1]);consume_input_fd(p[0],1280,800);close(p[0]);}
int main(void){g_scene.panel_height=38;g_r2_canvas_width=1280;g_r2_canvas_height=800;g_r7_controls_visible=2;int x,y;unsigned w,h;r22_quick_layout(1280,800,&x,&y,&w,&h);assert(r7_chrome_click(x+w-70,y+240)&&g_r7_controls_visible==5&&!g_power_focus);key(NOVA_KEY_ENTER);assert(!g_r7_controls_visible&&!g_power_status);g_r7_controls_visible=2;g_settings_focus=4;key(NOVA_KEY_TAB);assert(g_settings_focus==5);key(NOVA_KEY_ENTER);assert(g_r7_controls_visible==5&&!g_power_focus);key(NOVA_KEY_ESC);assert(!g_r7_controls_visible);g_r7_controls_visible=5;r46_power_request();assert(g_power_status==3&&g_power_fd<0);/* private path has no PID1; fail closed */
 for(unsigned theme=0;theme<3;theme++){unsigned w=1280,h=800;size_t bytes=(size_t)w*h*4;unsigned char*b=malloc(bytes+64);assert(b);memset(b,0x5a,bytes+64);g_session_theme.choice=theme;r7_controls(b+32,w,h,w*4);for(unsigned j=0;j<32;j++)assert(b[j]==0x5a&&b[bytes+32+j]==0x5a);free(b);}puts("POWER_UI_MOUSE_KEYBOARD_CANCEL_DEFAULT_ERROR_AND_BOUNDS_PASS");return 0;}
