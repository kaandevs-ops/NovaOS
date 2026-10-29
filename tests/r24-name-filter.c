#include "nova_files_ui.h"
#include "nova_files_surface_bridge.h"
#include "nova_input.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static void key(struct nova_files_surface_bridge*b,unsigned int code,unsigned int mods){struct nova_surface_event e={0};e.magic=NOVA_SURFACE_PROTOCOL_MAGIC;e.version=6;e.size=sizeof(e);e.client_id=3;e.surface_id=300;e.type=NOVA_SURFACE_EVENT_KEY;e.code=code;e.value=1;e.modifiers=mods;(void)nova_files_surface_bridge_event(b,&e);}
int main(void){
 struct nova_files_model m={0};strcpy(m.path,"/");m.count=5;
 const char*names[]={"Report-01.txt","report-02.TXT","Notes.txt","Documents","Çalışma-REPORT.txt"};
 for(unsigned int i=0;i<5;i++){strcpy(m.entries[i].name,names[i]);m.entries[i].type=i==3?4:8;}
 struct nova_files_model before=m;struct nova_files_ui u;nova_files_ui_init(&u,&m);struct nova_files_surface_bridge b;nova_files_surface_bridge_init(&b,&u,3,300,80,100);
 u.selected=0;key(&b,NOVA_KEY_F,NOVA_INPUT_MOD_CTRL);assert(u.search_focused);
 unsigned int codes[]={19,18,25,24,19,20};for(unsigned int i=0;i<6;i++)key(&b,codes[i],i==0?NOVA_INPUT_MOD_SHIFT:0);
 assert(!strcmp(u.name_filter,"Report")&&nova_files_ui_match_count(&u)==3&&u.selected==0&&u.sort_mode==0&&!u.folders_only);
 key(&b,NOVA_KEY_S,0);assert(nova_files_ui_match_count(&u)==0&&u.sort_mode==0);key(&b,NOVA_KEY_BACKSPACE,0);assert(nova_files_ui_match_count(&u)==3);
 key(&b,NOVA_KEY_ENTER,0);assert(!u.search_focused&&!strcmp(u.name_filter,"Report"));
 key(&b,NOVA_KEY_ESC,0);assert(!u.search_focused&&!u.name_filter[0]&&nova_files_ui_match_count(&u)==5);
 u.selected=2;u.properties_open=1;key(&b,NOVA_KEY_ESC,0);assert(!u.properties_open&&!strcmp(m.path,"/"));
 key(&b,NOVA_KEY_F,NOVA_INPUT_MOD_CTRL);for(unsigned int i=0;i<500;i++)nova_files_ui_filter_key(&u,'x');assert(strlen(u.name_filter)==63&&nova_files_ui_match_count(&u)==0);
 const unsigned int widths[]={640,800,1280,1920},heights[]={400,500,800,1080};
 for(unsigned int size=0;size<4;size++)for(unsigned int theme=0;theme<3;theme++){
  assert(nova_files_ui_resize(&u,widths[size],heights[size]));u.theme=theme;size_t words=(size_t)widths[size]*heights[size];unsigned int*p=malloc((words+32)*4);assert(p);for(unsigned int i=0;i<16;i++)p[i]=p[words+16+i]=0xdeadbeef;
  assert(nova_files_ui_render(&u,p+16,widths[size],heights[size],widths[size]*4));for(unsigned int i=0;i<16;i++)assert(p[i]==0xdeadbeef&&p[words+16+i]==0xdeadbeef);free(p);
 }
 key(&b,NOVA_KEY_ESC,0);nova_files_ui_find(&u);const char*literal="../";for(unsigned int i=0;literal[i];i++)nova_files_ui_filter_key(&u,(unsigned int)literal[i]);assert(nova_files_ui_match_count(&u)==0&&!memcmp(&m,&before,sizeof(m)));
 nova_files_ui_filter_key(&u,27);nova_files_ui_find(&u);nova_files_ui_filter_key(&u,'n');assert(nova_files_ui_match_count(&u)==2); /* Notes and Documents */
 assert(nova_files_ui_pointer(&u,50,350,1,1)>=0);assert(!u.search_focused); /* list/sidebar click defocuses editing */
 u.pressed_control=7;struct nova_surface_event e={0};e.magic=NOVA_SURFACE_PROTOCOL_MAGIC;e.version=6;e.size=sizeof(e);e.client_id=3;e.surface_id=300;e.type=NOVA_SURFACE_EVENT_POINTER;e.code=NOVA_BTN_LEFT;e.value=0;e.cursor_x=-100;e.cursor_y=-100;
 assert(nova_files_surface_bridge_event(&b,&e)>0&&!u.pressed_control);assert(!memcmp(&m,&before,sizeof(m)));
 puts("R24_REAL_NAME_FILTER_CTRL_F_ASCII_CASE_SELECTION_BOUNDS_LITERAL_PATH_NO_AUTHORITY_ESC_RELEASE_12_RENDER_GUARDS=PASS");return 0;
}
