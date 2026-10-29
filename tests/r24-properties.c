#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nova_files_ui.h"
int main(void){struct nova_files_model f;memset(&f,0,sizeof(f));f.count=2;f.depth=1;strcpy(f.path,"/Documents");strcpy(f.entries[0].name,"Real-metadata-fixture.txt");f.entries[0].type=8;strcpy(f.entries[1].name,"Folder");f.entries[1].type=4;struct nova_files_ui u;nova_files_ui_init(&u,&f);assert(!nova_files_ui_key(&u,'i'));assert(nova_files_ui_key(&u,36));u.metadata[0].valid=1;u.metadata[0].bytes=2048;assert(nova_files_ui_key(&u,'i')&&u.properties_open);unsigned int sizes[][2]={{640,400},{800,500},{1280,800},{1920,1080}};
 for(unsigned int j=0;j<4;j++)for(unsigned int t=0;t<3;t++){unsigned int w=sizes[j][0],h=sizes[j][1];assert(nova_files_ui_resize(&u,w,h));u.theme=t;size_t n=(size_t)w*h*4;unsigned char*b=malloc(n+64);assert(b);memset(b,0x71,n+64);assert(nova_files_ui_render(&u,b+32,w,h,w*4));for(int k=0;k<32;k++)assert(b[k]==0x71&&b[n+32+k]==0x71);free(b);}
 assert(nova_files_ui_key(&u,27)&&!u.properties_open&&f.depth==1&&u.selected==0);assert(nova_files_ui_key(&u,'i'));assert(nova_files_ui_pointer(&u,(int)u.viewport_width-28,120,1,1)&&!u.properties_open);assert(nova_files_ui_pointer(&u,(int)u.viewport_width-210,48,1,1)&&u.properties_open);assert(nova_files_ui_pointer(&u,(int)u.viewport_width-90,180,1,0)&&!u.pressed_control);u.selected=~0U;assert(nova_files_ui_key(&u,'i')&&!u.properties_open);assert(nova_files_ui_key(&u,'b')==0);puts("R24_REAL_INFO_CONTROL_SELECTION_READONLY_METADATA_ESC_NO_NAVIGATION_4_VIEWPORT_3_THEME_GUARDS=PASS");}
