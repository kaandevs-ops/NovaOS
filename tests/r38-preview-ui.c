#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nova_files_ui.h"
static int mode;
static int reader(int fd,const char*name,char*out,unsigned cap){
 assert(fd==42&&!strcmp(name,"notes.txt")&&cap==4097);
 if(mode<0)return mode;
 if(mode==0){out[0]=0;return 0;}
 memset(out,'x',4096);for(unsigned i=79;i<4096;i+=80)out[i]='\n';out[4096]=0;return 4096;
}
int main(void){
 struct nova_files_model m;memset(&m,0,sizeof m);m.count=1;m.directory_fds[0]=42;m.entries[0].type=8;strcpy(m.entries[0].name,"notes.txt");strcpy(m.path,"/");
 for(unsigned size=0;size<3;size++){
  unsigned w=size==0?640:size==1?800:1200,h=size==0?400:size==1?500:800;
  struct nova_files_ui u;nova_files_ui_init(&u,&m);u.preview_reader=reader;u.selected=0;assert(nova_files_ui_resize(&u,w,h));
  for(int j=0;j<3;j++){
   mode=j==0?4096:j==1?0:-13;assert(nova_files_ui_key(&u,13)&&u.preview_open);assert(u.preview_status==mode);
   size_t bytes=(size_t)w*h*4;unsigned char*b=malloc(bytes+64);assert(b);memset(b,0x5a,bytes+64);assert(nova_files_ui_render(&u,b,w,h,w*4));for(unsigned i=0;i<64;i++)assert(b[bytes+i]==0x5a);free(b);
   assert(nova_files_ui_pointer(&u,w-50,130,1,1)&&!u.preview_open);
  }
 }
 puts("PREVIEW_UI_ASAN_UBSAN_640_800_1200_RENDER_CLOSE_CLIPPING_PASS (unit callback fixture; not guest filesystem)");return 0;
}
