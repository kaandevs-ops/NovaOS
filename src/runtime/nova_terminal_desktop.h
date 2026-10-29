#ifndef NOVA_TERMINAL_DESKTOP_H
#define NOVA_TERMINAL_DESKTOP_H
/* Compositor-owned terminal surface. It has no fourth app principal and never
 * executes command strings. Text is forwarded to the private isolated helper. */
static void r25_terminal_layout(unsigned w,unsigned h){
 if(w<360||h<360)return;
 if(g_terminal.maximized){g_terminal.x=16;g_terminal.y=46;g_terminal.width=(int)w-32;g_terminal.height=(int)h-140;}
 if(!g_terminal.placed){g_terminal.width=w>1050?960:(int)w-64;g_terminal.height=h>720?530:(int)h-160;g_terminal.x=((int)w-g_terminal.width)/2;g_terminal.y=72;g_terminal.placed=1;}
 if(g_terminal.width>(int)w-32)g_terminal.width=(int)w-32;if(g_terminal.width<320)g_terminal.width=320;
 if(g_terminal.height>(int)h-140)g_terminal.height=(int)h-140;if(g_terminal.height<204)g_terminal.height=204;
 if(g_terminal.x<16)g_terminal.x=16;if(g_terminal.x+g_terminal.width>(int)w-16)g_terminal.x=(int)w-16-g_terminal.width;
 if(g_terminal.y<46)g_terminal.y=46;if(g_terminal.y+g_terminal.height>(int)h-94)g_terminal.y=(int)h-94-g_terminal.height;
 unsigned cols=(unsigned)(g_terminal.width-32)/9,rows=(unsigned)(g_terminal.height-110)/18;
 if(cols>NOVA_TERM_MAX_COLS)cols=NOVA_TERM_MAX_COLS;if(rows>NOVA_TERM_MAX_ROWS)rows=NOVA_TERM_MAX_ROWS;
 if(cols<20)cols=20;if(rows<4)rows=4;
 if(g_terminal.cols!=cols||g_terminal.rows!=rows){g_terminal.cols=cols;g_terminal.rows=rows;g_terminal.resize_pending=1;nova_term_grid_resize(&g_terminal.grid,rows,cols);}
}
/* Session/FD unchanged: maximize changes only compositor-owned geometry. */
static void r43_terminal_maximize(unsigned w,unsigned h){
 if(w<360||h<360)return;
 if(!g_terminal.maximized){g_terminal.restore_x=g_terminal.x;g_terminal.restore_y=g_terminal.y;g_terminal.restore_width=g_terminal.width;g_terminal.restore_height=g_terminal.height;g_terminal.maximized=1;}
 else{g_terminal.maximized=0;g_terminal.x=g_terminal.restore_x;g_terminal.y=g_terminal.restore_y;g_terminal.width=g_terminal.restore_width;g_terminal.height=g_terminal.restore_height;}
 g_terminal.grab=0;r25_terminal_layout(w,h);g_scene.visual_generation++;
}
static unsigned int top_client_surface_at(int,int);
static int r25_terminal_at(int x,int y){
 if(!g_terminal.visible||x<g_terminal.x||x>=g_terminal.x+g_terminal.width||y<g_terminal.y||y>=g_terminal.y+g_terminal.height)return 0;
 unsigned id=top_client_surface_at(x,y);struct nova_client_surface *s=id?find_client_surface(id):(struct nova_client_surface *)0;
 return !s||s->z_index<g_terminal.z_index;
}
#include "nova_terminal_font.h"
static void r25_terminal_draw(unsigned char *b,unsigned w,unsigned h,unsigned pitch){
 if(!g_terminal.visible||w<360||h<360)return;r25_terminal_layout(w,h);
 struct nova_ui_canvas c={b,w,h,pitch};struct nova_ui_palette p=nova_ui_palette_for(g_session_theme.choice);
 int x=g_terminal.x,y=g_terminal.y,ww=g_terminal.width,hh=g_terminal.height;
 r19_shadow(b,w,h,pitch,x,y,(unsigned)ww,(unsigned)hh,30,1);r8_surface(b,w,h,pitch,x,y,(unsigned)ww,(unsigned)hh,14,p.paper,g_terminal.focused?p.accent:p.line);
 nova_ui_text(c,x+20,y+28,NOVA_UI_HEADING,1,p.ink,g_terminal_caps?"Nova Terminal · Türkçe Q · CAPS":"Nova Terminal · Türkçe Q",x+16,y+6,(unsigned)ww-160,32);
 char subtitle[96];nova_ui_ellipsis(g_terminal.persistent?"Persistent workspace / shared with Files":ww<500?"Session-only workspace":"Isolated session / temporary workspace",subtitle,sizeof subtitle,NOVA_UI_CAPTION,(unsigned)ww-40);nova_ui_text(c,x+20,y+53,NOVA_UI_CAPTION,1,p.muted,subtitle,x+16,y+34,(unsigned)ww-32,24);
 int hover=g_render.cursor_x>=x+ww-100&&g_render.cursor_x<x+ww-72&&g_render.cursor_y>=y+8&&g_render.cursor_y<y+36;
 nova_ui_button(c,x+ww-100,y+8,28,28,p,hover?NOVA_UI_HOVER:0);
 nova_ui_line(c,x+ww-92,y+18,x+ww-80,y+18,p.ink);nova_ui_line(c,x+ww-92,y+18,x+ww-92,y+29,p.ink);nova_ui_line(c,x+ww-80,y+18,x+ww-80,y+29,p.ink);nova_ui_line(c,x+ww-92,y+29,x+ww-80,y+29,p.ink);
 if(g_terminal.maximized){nova_ui_line(c,x+ww-89,y+15,x+ww-77,y+15,p.ink);nova_ui_line(c,x+ww-77,y+15,x+ww-77,y+26,p.ink);}
 hover=g_render.cursor_x>=x+ww-66&&g_render.cursor_y>=y+8&&g_render.cursor_x<x+ww-40&&g_render.cursor_y<y+36;
 nova_ui_button(c,x+ww-68,y+8,28,28,p,hover?NOVA_UI_HOVER:0);nova_ui_line(c,x+ww-59,y+23,x+ww-49,y+23,p.ink);
 hover=g_render.cursor_x>=x+ww-36&&g_render.cursor_y>=y+8&&g_render.cursor_x<x+ww-8&&g_render.cursor_y<y+36;
 nova_ui_button(c,x+ww-36,y+8,28,28,p,hover?NOVA_UI_HOVER:0);nova_ui_line(c,x+ww-27,y+18,x+ww-17,y+28,p.ink);nova_ui_line(c,x+ww-17,y+18,x+ww-27,y+28,p.ink);
 nova_ui_round(c,x+12,y+64,ww-24,hh-104,8,g_session_theme.choice==0?0x00f4f6fa:0x00161e29);
 unsigned ink=g_session_theme.choice==0?0x00222b3c:0x00e5efec;
 for(unsigned row=0;row<g_terminal.grid.rows;row++){
  const unsigned *line=nova_term_view_row(&g_terminal.grid,row);
  for(unsigned col=0;col<g_terminal.grid.cols;col++)if(line[col]!=' '&&line[col]){nova_term_glyph(c,x+16+(int)col*9,y+68+(int)row*18,line[col],ink);}
 }
 /* Raster is clipped to the same owned terminal content; never desktop chrome. */
 if(g_terminal.graphics->visible&&!g_terminal.grid.scroll){const struct nova_sixel*s=g_terminal.graphics;
  unsigned ox=s->anchor_x*9U,oy=s->anchor_y*18U,cw=g_terminal.grid.cols*9U,ch=g_terminal.grid.rows*18U;
  for(unsigned yy=0;yy<s->height&&oy+yy<ch;yy++){unsigned py=(unsigned)(y+68)+oy+yy;if(py>=h)break;
   for(unsigned xx=0;xx<s->width&&ox+xx<cw;xx++){unsigned px=(unsigned)(x+16)+ox+xx;if(px<w)((unsigned*)(b+(unsigned long)py*pitch))[px]=nova_sixel_color(s,xx,yy);}
  }
 }
 if(g_terminal.ready&&g_terminal.focused&&!g_terminal.grid.scroll){nova_ui_round(c,x+16+(int)g_terminal.grid.x*9,y+84+(int)g_terminal.grid.y*18,8,2,0,p.accent);}
 const char *status=g_terminal.error?"Session unavailable / close and retry":g_terminal.ended?"Session ended / close and reopen":!g_terminal.ready?"Starting isolated shell...":g_terminal.backpressure?"Input queue full / wait before typing":g_terminal.grid.scroll?"Scrollback / Page Down returns to current output":"Input: active program   Ctrl+C: interrupt   Page Up/Down: scroll";
 char hint[128];if(g_terminal.ready&&!g_terminal.backpressure&&!g_terminal.grid.scroll&&ww<640)status="Input: active program   Ctrl+C: interrupt";nova_ui_ellipsis(status,hint,sizeof hint,NOVA_UI_CAPTION,(unsigned)ww-48);nova_ui_text(c,x+16,y+hh-17,NOVA_UI_CAPTION,1,p.muted,hint,x+12,y+hh-34,(unsigned)ww-32,26);
 nova_ui_line(c,x+ww-18,y+hh-10,x+ww-10,y+hh-18,p.muted);nova_ui_line(c,x+ww-24,y+hh-10,x+ww-10,y+hh-24,p.muted);
}
static int r25_terminal_click(int x,int y){
 if(g_r2_canvas_width>=700&&x>=(int)g_r2_canvas_width-238&&x<(int)g_r2_canvas_width-136&&y>=0&&y<38){nova_term_toggle();g_r7_controls_visible=0;g_r7_library_visible=0;g_scene.dragging=0;g_scene.visual_generation++;return 1;}
 if(!g_terminal.visible)return 0;
 if(!r25_terminal_at(x,y)){g_terminal.focused=0;g_terminal.grab=0;g_scene.visual_generation++;return 0;}
 r25_terminal_focus();
 if(y<g_terminal.y+38&&x>=g_terminal.x+g_terminal.width-36){nova_term_disconnect(0);g_terminal.visible=0;}
 else if(y<g_terminal.y+38&&x>=g_terminal.x+g_terminal.width-68)g_terminal.visible=0;
 else if(y<g_terminal.y+38&&x>=g_terminal.x+g_terminal.width-100)r43_terminal_maximize(g_r2_canvas_width,g_r2_canvas_height);
 else if(!g_terminal.maximized&&y>=g_terminal.y+g_terminal.height-24&&x>=g_terminal.x+g_terminal.width-24)g_terminal.grab=2;
 else if(!g_terminal.maximized&&y<g_terminal.y+38)g_terminal.grab=1;
 g_terminal.grab_x=x;g_terminal.grab_y=y;g_scene.dragging=0;g_scene.visual_generation++;return 1;
}
static int r25_terminal_motion(int x,int y,unsigned w,unsigned h){
 if(!g_terminal.visible||!g_terminal.grab)return 0;
 int dx=x-g_terminal.grab_x,dy=y-g_terminal.grab_y;g_terminal.grab_x=x;g_terminal.grab_y=y;
 if(g_terminal.grab==1){g_terminal.x+=dx;g_terminal.y+=dy;}else{g_terminal.width+=dx;g_terminal.height+=dy;}
 r25_terminal_layout(w,h);return 1;
}
#endif
