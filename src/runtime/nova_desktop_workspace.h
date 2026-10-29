#ifndef NOVA_DESKTOP_WORKSPACE_H
#define NOVA_DESKTOP_WORKSPACE_H
#include "nova_shell_quote.h"
#include "nova_desktop_file_ui.h"
static char g_desktop_drag_path[96];
static int g_desktop_drag_x,g_desktop_drag_y;
static unsigned g_desktop_double_click;
static unsigned g_desktop_menu,g_desktop_menu_choice;
static int r48_desktop_menu_open(void){return g_desktop_menu!=0||g_desktop_document_visible||g_desktop_trash_visible;}
static int g_desktop_menu_x,g_desktop_menu_y;
static void r48_desktop_request(unsigned action){
 if(g_desktop_pending>1)return;
 if(g_terminal.fd<0&&!nova_term_start())return;
 g_desktop_pending=action;nova_term_flush();g_scene.visual_generation++;
}
static int r48_desktop_item_at(int x,int y,unsigned w,unsigned h){
 (void)w;unsigned rows=h>200?(h-170)/100:0;if(!rows)return -1;
 for(unsigned i=0;i<g_desktop_files.count;i++){int ix=28+(int)(i/rows)*112,iy=58+(int)(i%rows)*100;if(x>=ix&&x<ix+100&&y>=iy&&y<iy+90)return (int)i;}return -1;
}
static void r48_desktop_draw(unsigned char*b,unsigned w,unsigned h,unsigned pitch){
 if(w<360||h<300)return;struct nova_ui_canvas c={b,w,h,pitch};struct nova_ui_palette p=nova_ui_palette_for(g_session_theme.choice);unsigned rows=(h-170)/100;
 for(unsigned i=0;i<g_desktop_files.count;i++){int x=28+(int)(i/rows)*112,y=58+(int)(i%rows)*100;if(x+100>(int)w-20)break;
  if(g_desktop_selected==i)nova_ui_round(c,x,y,100,90,10,0x00395367);
  if(g_desktop_files.entries[i].kind==2)nova_ui_desktop_folder(c,x+24,y+2,52,p.accent,g_desktop_selected==i);
  else{nova_ui_round(c,x+34,y+8,32,40,4,0x00eef4fa);nova_ui_line(c,x+41,y+20,x+59,y+20,0x00608096);nova_ui_line(c,x+41,y+28,x+59,y+28,0x00608096);}
  char label[80];nova_ui_ellipsis(g_desktop_files.entries[i].name,label,sizeof label,NOVA_UI_BODY,96);nova_ui_text(c,x+4,y+72,NOVA_UI_BODY,1,0x00ffffff,label,x,y+52,100,30);
 }
 /* A real trash control, not a fake application task or fourth principal. */
 int tx=(int)w-124,ty=(int)h-202;
 nova_ui_round(c,tx+31,ty+16,38,43,6,p.paper);nova_ui_round(c,tx+26,ty+12,48,5,2,p.line);nova_ui_round(c,tx+40,ty+5,20,6,3,p.paper);nova_ui_line(c,tx+43,ty+25,tx+43,ty+48,p.muted);nova_ui_line(c,tx+57,ty+25,tx+57,ty+48,p.muted);
 nova_ui_text(c,tx+6,ty+80,NOVA_UI_BODY,1,0x00ffffff,"Çöp Kutusu",tx,ty+60,100,30);
 if(g_desktop_file_error&&!g_desktop_document_visible&&!g_desktop_trash_visible)nova_ui_text(c,28,(int)h-132,NOVA_UI_CAPTION,1,p.ink,r54_desktop_error(g_desktop_file_error),20,(int)h-154,w-40,30);
 if(g_desktop_files.clipped)nova_ui_text(c,28,(int)h-108,NOVA_UI_CAPTION,1,p.ink,"Masaüstü liste sınırı: 24 öğe. Tümü için Files.",20,(int)h-130,w-40,30);
 if(g_desktop_files.status)nova_ui_text(c,28,(int)h-108,NOVA_UI_CAPTION,1,p.ink,"Desktop operation failed; existing files preserved",20,(int)h-130,w-40,30);
}
static void r48_desktop_menu_draw(unsigned char*b,unsigned w,unsigned h,unsigned pitch){
 if(!g_desktop_menu)return;struct nova_ui_canvas c={b,w,h,pitch};struct nova_ui_palette p=nova_ui_palette_for(g_session_theme.choice);int x=g_desktop_menu_x,y=g_desktop_menu_y;
 r19_shadow(b,w,h,pitch,x,y,244,296,18,1);nova_ui_round(c,x,y,244,296,12,p.paper);
 const char*labels[]={"Yeni metin dosyası","Yeni klasör · Ctrl+Shift+N","Masaüstünü yenile · F5","Dosyaları aç","Terminali aç","Seçili öğeyi aç","Çöp kutusuna taşı","Çöp kutusunu aç"};
 for(unsigned i=0;i<8;i++){int yy=y+8+(int)i*35;unsigned state=i==g_desktop_menu_choice?NOVA_UI_FOCUSED:0;if((i==5||i==6)&&(g_desktop_selected>=g_desktop_files.count||r54_desktop_busy()))state|=NOVA_UI_DISABLED;int hover=g_render.cursor_x>=x+8&&g_render.cursor_x<x+236&&g_render.cursor_y>=yy&&g_render.cursor_y<yy+34;if(hover)state|=NOVA_UI_HOVER;nova_ui_button(c,x+8,yy,228,34,p,state);nova_ui_text(c,x+19,yy+23,NOVA_UI_BODY,1,p.ink,labels[i],x+12,yy,220,34);}
}
static void r48_desktop_activate(unsigned choice){
 g_desktop_menu=0;
 if(choice<3)r48_desktop_request(choice==0?2:choice==1?3:1);
 else if(choice==3)(void)send_desktop_task_event(3,client_surface_count_for(3)?NOVA_DESKTOP_ACTION_ACTIVATE:NOVA_DESKTOP_ACTION_LAUNCH);
 else if(choice==4){if(!g_terminal.visible||!g_terminal.focused){nova_term_toggle();set_client_focus(0);}}
 else if(choice==5)r54_desktop_open_selected();
 else if(choice==6&&g_desktop_selected_name[0])(void)r54_desktop_request(NOVA_DESKTOP_TRASH_MOVE,g_desktop_selected_name,0);
 else if(choice==7)r54_desktop_show_trash();
 g_scene.visual_generation++;
}
static int r48_desktop_click(int x,int y){
 if(r54_desktop_overlay_click(x,y))return 1;
 if(g_desktop_menu){int lx=x-g_desktop_menu_x,ly=y-g_desktop_menu_y-8;if(lx>=8&&lx<236&&ly>=0&&ly<280)r48_desktop_activate((unsigned)ly/35);else{g_desktop_menu=0;g_scene.visual_generation++;}return 1;}
 if(top_client_surface_at(x,y)||r25_terminal_at(x,y)||r19_overlay_at(x,y)||y<48||y>=(int)g_r2_canvas_height-104){g_desktop_has_focus=0;return 0;}
 g_desktop_has_focus=1;g_terminal.focused=0;set_client_focus(0);
 if(r54_desktop_trash_at(x,y)){r54_desktop_show_trash();return 1;}
 int i=r48_desktop_item_at(x,y,g_r2_canvas_width,g_r2_canvas_height);g_desktop_selected=i<0?~0U:(unsigned)i;
 if(i<0)g_desktop_selected_name[0]=0;
 if(i>=0){nova_copy(g_desktop_selected_name,g_desktop_files.entries[i].name,nova_strlen(g_desktop_files.entries[i].name)+1);const char*name=g_desktop_files.entries[i].name;unsigned n=0;const char*prefix="/home/nova/Desktop/";for(;prefix[n];n++)g_desktop_drag_path[n]=prefix[n];unsigned j=0;for(;name[j]&&n+1<sizeof g_desktop_drag_path;j++)g_desktop_drag_path[n++]=name[j];g_desktop_drag_path[n]=0;g_desktop_drag_x=x;g_desktop_drag_y=y;
 unsigned long long now=nova_render_clock(1);g_desktop_double_click=nova_streq(g_desktop_last_click_name,name)&&now-g_desktop_last_click_ns<=450000000ULL;nova_copy(g_desktop_last_click_name,name,nova_strlen(name)+1);g_desktop_last_click_ns=now;
 }g_scene.visual_generation++;return i>=0;
}
static int r49_desktop_drop(int x,int y){
 if(!g_desktop_drag_path[0])return 0;
 int dx=x-g_desktop_drag_x,dy=y-g_desktop_drag_y;char quoted[400];
 if(dx*dx+dy*dy>=64&&r54_desktop_trash_at(x,y)&&!top_client_surface_at(x,y)&&!r25_terminal_at(x,y)&&!r19_overlay_at(x,y)&&!g_desktop_document_visible&&!g_desktop_trash_visible){int sent=r54_desktop_request(NOVA_DESKTOP_TRASH_MOVE,g_desktop_drag_path+19,0);g_desktop_drag_path[0]=0;g_desktop_double_click=0;g_desktop_last_click_name[0]=0;return sent;}
 if(dx*dx+dy*dy<64){g_desktop_drag_path[0]=0;if(g_desktop_double_click){g_desktop_double_click=0;g_desktop_last_click_name[0]=0;r54_desktop_open_selected();return 1;}return 0;}
 g_desktop_double_click=0;g_desktop_last_click_name[0]=0;
 unsigned n=nova_shell_quote(g_desktop_drag_path,quoted,sizeof quoted);g_desktop_drag_path[0]=0;
 if(dx*dx+dy*dy<64||!r25_terminal_at(x,y)||g_desktop_menu||g_r7_controls_visible||g_r7_library_visible||!g_terminal.ready||g_terminal.fd<0||!n)return 0;
 if(n>sizeof g_terminal.input-g_terminal.queued){g_terminal.backpressure=1;return 0;}
 for(unsigned i=0;i<n;i++)g_terminal.input[g_terminal.queued++]=(unsigned char)quoted[i];
 r25_terminal_focus();g_terminal.grid.scroll=0;nova_term_flush();g_scene.visual_generation++;return 1;
}
static int r59_desktop_navigation(unsigned key){
 if(!g_desktop_has_focus)return 0;
 if(key==63){r48_desktop_request(1);return 1;} /* F5 */
 if(key==49&&(g_input_modifiers&NOVA_INPUT_MOD_CTRL)&&(g_input_modifiers&NOVA_INPUT_MOD_SHIFT)){r48_desktop_request(3);return 1;}
 if(key!=NOVA_KEY_UP&&key!=NOVA_KEY_DOWN&&key!=105&&key!=106&&key!=NOVA_KEY_HOME&&key!=107)return 0;
 unsigned rows=g_r2_canvas_height>200?(g_r2_canvas_height-170)/100:0;
 unsigned cols=g_r2_canvas_width>48?(g_r2_canvas_width-48)/112:0;
 unsigned count=g_desktop_files.count,visible=rows*cols;if(count>visible)count=visible;
 if(!count)return 1;unsigned i=g_desktop_selected;
 if(i>=count)i=0;else if(key==NOVA_KEY_HOME)i=0;else if(key==107)i=count-1;
 else if(key==NOVA_KEY_UP){if(i%rows)i--;}
 else if(key==NOVA_KEY_DOWN){if(i%rows+1<rows&&i+1<count)i++;}
 else if(key==105){if(i>=rows)i-=rows;}
 else if(key==106){if(i+rows<count)i+=rows;}
 g_desktop_selected=i;nova_copy(g_desktop_selected_name,g_desktop_files.entries[i].name,nova_strlen(g_desktop_files.entries[i].name)+1);
 g_desktop_drag_path[0]=0;g_desktop_double_click=0;g_desktop_last_click_name[0]=0;g_scene.visual_generation++;return 1;
}
static int r48_desktop_key(unsigned key){
 if(r54_desktop_overlay_key(key))return 1;
 if(!g_desktop_menu){if((g_terminal.visible&&g_terminal.focused)||g_client.focus_surface||g_r7_controls_visible||g_r7_library_visible)return 0;if(r59_desktop_navigation(key))return 1;if(g_desktop_has_focus&&key==NOVA_KEY_ENTER){r54_desktop_open_selected();return 1;}if(g_desktop_has_focus&&key==111&&g_desktop_selected_name[0]){(void)r54_desktop_request(NOVA_DESKTOP_TRASH_MOVE,g_desktop_selected_name,0);return 1;}return 0;}if(key==NOVA_KEY_ESC)g_desktop_menu=0;else if(key==NOVA_KEY_DOWN||key==NOVA_KEY_TAB)g_desktop_menu_choice=(g_desktop_menu_choice+1)%8;else if(key==NOVA_KEY_UP)g_desktop_menu_choice=(g_desktop_menu_choice+7)%8;else if(key==NOVA_KEY_ENTER)r48_desktop_activate(g_desktop_menu_choice);g_scene.visual_generation++;return 1;
}
#endif
