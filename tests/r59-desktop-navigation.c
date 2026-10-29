#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
int main(void){g_r2_canvas_width=1600;g_r2_canvas_height=900;g_desktop_has_focus=1;g_desktop_files.count=20;for(unsigned i=0;i<20;i++)snprintf(g_desktop_files.entries[i].name,64,"Item-%02u",i);g_desktop_selected=~0U;
 assert(r48_desktop_key(NOVA_KEY_DOWN)&&g_desktop_selected==0);assert(r48_desktop_key(NOVA_KEY_DOWN)&&g_desktop_selected==1);assert(r48_desktop_key(106)&&g_desktop_selected==8);assert(r48_desktop_key(105)&&g_desktop_selected==1);assert(r48_desktop_key(NOVA_KEY_HOME)&&g_desktop_selected==0);assert(r48_desktop_key(NOVA_KEY_UP)&&g_desktop_selected==0);assert(r48_desktop_key(107)&&g_desktop_selected==19);assert(nova_streq(g_desktop_selected_name,"Item-19"));assert(r48_desktop_key(NOVA_KEY_DOWN)&&g_desktop_selected==19);
 g_r2_canvas_width=360;g_r2_canvas_height=400;assert(r48_desktop_key(107)&&g_desktop_selected==0);assert(r48_desktop_key(107)&&g_desktop_selected==3);assert(r48_desktop_key(106)&&g_desktop_selected==3);
 g_desktop_files.count=0;assert(r48_desktop_key(NOVA_KEY_DOWN));g_desktop_files.count=20;g_terminal.visible=1;g_terminal.focused=1;assert(!r48_desktop_key(NOVA_KEY_DOWN));g_terminal.focused=0;g_desktop_has_focus=0;assert(!r48_desktop_key(NOVA_KEY_DOWN));puts("DESKTOP_NAVIGATION_AND_FOCUS_PASS");return 0;}
