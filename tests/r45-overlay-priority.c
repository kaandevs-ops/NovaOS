#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
int main(void){
 g_scene.panel_height=38;g_r2_canvas_width=1280;g_r2_canvas_height=800;
 g_terminal.visible=1;g_terminal.focused=0;r25_terminal_layout(1280,800);
 g_session_theme.choice=2;g_r7_controls_visible=2;
 int x,y;unsigned w,h;r22_quick_layout(1280,800,&x,&y,&w,&h);
 struct nova_ui_rect q=nova_ui_segment_rect(x+16,y+76,(int)w-32,74,3,6,1);
 int px=q.x+q.w/2,py=q.y+q.h/2;
 assert(r25_terminal_at(px,py));
 assert(r7_chrome_click(px,py));
 assert(g_session_theme.choice==1);assert(!g_terminal.focused);assert(g_r7_controls_visible==2);
 /* An outside click closes the overlay and can focus the visible terminal. */
 assert(r7_chrome_click(600,180));assert(!g_r7_controls_visible);assert(g_terminal.focused);
 puts("R45_FRONTMOST_OVERLAY_CLICK_BEFORE_TERMINAL=PASS");return 0;
}
