#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "nova_files_ui.h"
#include "nova_files_font.h"
#include "nova_files_surface_bridge.h"
#include "nova_files_guest_adapter.h"
#include "nova_input.h"
#include "nova_ui.h"
int main(void){
 char text[132];nova_files_font_ellipsis("A very long filename that will not fit.txt",text,sizeof(text),0,90);
 assert(strstr(text,"\xe2\x80\xa6")&&strlen(text)<40);
 nova_files_font_ellipsis("abc",text,sizeof(text),0,0);assert(!*text);
 nova_files_font_ellipsis("\xc3\xa7",text,sizeof(text),0,100);assert(!strcmp(text,"\xc3\xa7"));
 for(unsigned int cap=1;cap<132;cap++)for(unsigned int width=0;width<600;width+=7){
  memset(text,0x5a,sizeof(text));nova_files_font_ellipsis("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz",text,cap,0,width);
  assert(memchr(text,0,cap));if(cap<132)assert(text[cap]==0x5a);
 }
 char path[]="/tmp/nova-r18-visual-XXXXXX";assert(mkdtemp(path));int fd=open(path,O_RDONLY|O_DIRECTORY);assert(fd>=0);
 struct nova_files_model model;assert(!nova_files_init(&model,fd,&nova_files_guest_ops,0));assert(!nova_files_list(&model));
 struct nova_files_ui ui;nova_files_ui_init(&ui,&model);
 assert(nova_files_ui_pointer(&ui,240,85,1,1)==0&&!ui.last_error&&model.depth==0);
 model.count=1;strcpy(model.entries[0].name,"A long bounded file name that must respect the type column.txt");model.entries[0].type=8;
 struct nova_files_surface_bridge bridge;nova_files_surface_bridge_init(&bridge,&ui,3,300,0,0);
 struct nova_surface_event e={0};e.magic=NOVA_SURFACE_PROTOCOL_MAGIC;e.version=NOVA_SURFACE_PROTOCOL_VERSION;e.size=sizeof(e);e.client_id=3;e.surface_id=300;e.type=NOVA_SURFACE_EVENT_POINTER;e.code=NOVA_REL_Y;e.value=1;e.cursor_x=300;e.cursor_y=145;
 assert(nova_files_surface_bridge_event(&bridge,&e)==1&&ui.hovered==0&&model.depth==0);
 assert(nova_files_surface_bridge_event(&bridge,&e)==0);
 e.cursor_x=-1;assert(nova_files_surface_bridge_event(&bridge,&e)==1&&ui.hovered==~0U);
 model.count=26;
 assert(nova_files_ui_pointer(&ui,788,451,1,1)==1&&ui.scroll==1);
 assert(nova_files_ui_pointer(&ui,788,148,1,1)==1&&ui.scroll==0);
 model.count=1;
 unsigned int sizes[][2]={{800,500},{1200,800},{1920,1080}};
 for(unsigned int i=0;i<3;i++){
  unsigned int w=sizes[i][0],h=sizes[i][1],pitch=w*4+32;size_t bytes=(size_t)pitch*h;
  unsigned char*p=malloc(bytes+64);assert(p);memset(p,0x73,bytes+64);assert(nova_files_ui_resize(&ui,w,h));
  assert(nova_files_ui_render(&ui,p+32,w,h,pitch));
  for(unsigned int k=0;k<32;k++)assert(p[k]==0x73&&p[bytes+32+k]==0x73);
  for(unsigned int y=0;y<h;y++)for(unsigned int k=w*4;k<pitch;k++)assert(p[32+(size_t)y*pitch+k]==0x73);
  free(p);
 }
 unsigned int pixels[64*64]={0};struct nova_ui_canvas canvas={(unsigned char*)pixels,64,64,256};
 nova_ui_round(canvas,-5,-5,40,40,12,0xffffff);nova_ui_line(canvas,20,20,30,30,0xffffff);
 unsigned int intermediate=0;for(unsigned int k=0;k<64*64;k++)if(pixels[k]&&pixels[k]!=0xffffff)intermediate++;
 assert(intermediate>0);assert(pixels[63*64+63]==0);
 nova_files_close(&model);assert(!rmdir(path));
 puts("R18_METRIC_ELLIPSIS_UTF8_FALLBACK_OUTPUT_BOUNDS=PASS");
 puts("R18_HOVER_DEDUP_LEAVE_DISABLED_ROOT_CONTROL=PASS");
 puts("R18_THREE_VIEWPORTS_PITCH_REDZONES_AA_CLIPPING=PASS");return 0;
}
