#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "nova_files_ui.h"
#include "nova_files_guest_adapter.h"
int main(void){
 char path[]="/tmp/nova-r21-view-XXXXXX";assert(mkdtemp(path));int fd=open(path,O_RDONLY|O_DIRECTORY);assert(fd>=0);
 for(unsigned int i=0;i<3;i++)assert(!mkdirat(fd,((const char*[]){"Alpha","Beta","Zeta"})[i],0700));
 for(unsigned int i=0;i<20;i++){char name[32];snprintf(name,sizeof(name),"File-%02u.txt",i);int f=openat(fd,name,O_WRONLY|O_CREAT|O_EXCL,0600);assert(f>=0);close(f);}assert(!symlinkat("/etc",fd,"Link"));assert(!mkfifoat(fd,"Pipe",0600));
 struct nova_files_model m;assert(!nova_files_init(&m,fd,&nova_files_guest_ops,0)&&!nova_files_list(&m)&&m.count==25);struct nova_files_model before=m;struct nova_files_ui ui;nova_files_ui_init(&ui,&m);
 assert(nova_files_ui_key(&ui,'f')&&ui.folders_only);assert(nova_files_ui_key(&ui,35)&&m.entries[ui.selected].type==4);assert(nova_files_ui_key(&ui,'s')&&ui.sort_mode==1);assert(nova_files_ui_key(&ui,36)&&!strcmp(m.entries[ui.selected].name,"Zeta"));
 assert(!memcmp(&m,&before,sizeof(m)));assert(nova_files_ui_key(&ui,'f')&&!ui.folders_only);assert(nova_files_ui_key(&ui,'s')&&ui.sort_mode==2);assert(nova_files_ui_key(&ui,36)&&m.entries[ui.selected].type==1);
 assert(nova_files_ui_pointer(&ui,270,116,1,1)>0&&ui.sort_mode==1);assert(nova_files_ui_pointer(&ui,710,116,1,1)>0&&ui.sort_mode==2);
 assert(nova_files_ui_pointer(&ui,650,48,1,1)>0&&ui.folders_only);assert(nova_files_ui_pointer(&ui,650,48,1,0)>0);assert(nova_files_ui_key(&ui,36)&&m.entries[ui.selected].type==4);
 unsigned int sizes[][2]={{640,400},{800,500},{1280,800},{1920,1080}};
 for(unsigned int a=0;a<4;a++)for(unsigned int theme=0;theme<3;theme++)for(unsigned int mode=0;mode<3;mode++){
  unsigned int w=sizes[a][0],h=sizes[a][1],pitch=w*4+16;size_t bytes=(size_t)pitch*h;unsigned char*b=malloc(bytes+64);assert(b);memset(b,0x47,bytes+64);ui.theme=theme;ui.sort_mode=mode;assert(nova_files_ui_resize(&ui,w,h));assert(nova_files_ui_render(&ui,b+32,w,h,pitch));for(unsigned int k=0;k<32;k++)assert(b[k]==0x47&&b[bytes+32+k]==0x47);for(unsigned int y=0;y<h;y++)for(unsigned int k=w*4;k<pitch;k++)assert(b[32+(size_t)y*pitch+k]==0x47);free(b);
 }
 assert(!memcmp(&m,&before,sizeof(m)));nova_files_close(&m);puts("R21_REAL_DIRECTORY_FOLDER_FILTER_NAME_TYPE_SORT_MODEL_IMMUTABILITY_KEYS_PIXEL_BOUNDS=PASS");return 0;
}
