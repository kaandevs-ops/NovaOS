#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "nova_ui_font.h"
#include "nova_files_ui.h"
#include "nova_files_guest_adapter.h"
static void ppm(const char*name,unsigned int*p,unsigned int w,unsigned int h){FILE*f=fopen(name,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(unsigned int i=0;i<w*h;i++){unsigned char b[3]={(unsigned char)(p[i]>>16),(unsigned char)(p[i]>>8),(unsigned char)p[i]};assert(fwrite(b,1,3,f)==3);}fclose(f);}
int main(int argc,char**argv){
 assert(argc==2);assert(nova_ui_measure("WWW",0,1)>nova_ui_measure("iii",0,1));
 assert(nova_ui_faces[0].glyphs[nova_ui_glyph_index(0x131)].cp==0x131);
 assert(nova_ui_faces[0].glyphs[nova_ui_glyph_index(0x1f680)].cp=='?');
 const char*tr="Çalışmalar · İstanbul · Öğrenci · Şehir";char fitted[132];nova_ui_ellipsis(tr,fitted,sizeof(fitted),0,600);assert(!strcmp(tr,fitted));
 for(unsigned int cap=1;cap<132;cap++)for(unsigned int width=0;width<240;width+=9){
  memset(fitted,0x71,sizeof(fitted));nova_ui_ellipsis(tr,fitted,cap,0,width);assert(memchr(fitted,0,cap));if(cap<132)assert(fitted[cap]==0x71);assert(nova_ui_measure(fitted,0,1)<=width);
  unsigned int pos=0;while(fitted[pos]){unsigned int cp,n=nova_ui_utf8((const unsigned char*)fitted+pos,132-pos,&cp);assert(n);pos+=n;}
 }
 unsigned int cp;assert(nova_ui_utf8((const unsigned char*)"\xf0",1,&cp)==1&&cp=='?');
 assert(nova_ui_utf8((const unsigned char*)"\xed\xa0\x80",3,&cp)==1&&cp=='?');
 char dir[]="/tmp/nova-r19-design-XXXXXX";assert(mkdtemp(dir));int fd=open(dir,O_RDONLY|O_DIRECTORY);assert(fd>=0);assert(!mkdirat(fd,"Documents",0700));assert(!mkdirat(fd,"Çalışmalar",0700));assert(!symlinkat("/etc",fd,"escape"));
 struct nova_files_model model;assert(!nova_files_init(&model,fd,&nova_files_guest_ops,0)&&!nova_files_list(&model));struct nova_files_ui ui;nova_files_ui_init(&ui,&model);assert(ui.place_count==2);
 assert(nova_files_ui_pointer(&ui,70,200,1,1)==1&&model.depth==1);assert(nova_files_ui_key(&ui,8)==1&&model.depth==0);
 strcpy(ui.places[0],"../escape");assert(nova_files_ui_pointer(&ui,70,200,1,1)==-1&&model.depth==0);assert(nova_files_ui_key(&ui,'r')==1);
 assert(nova_files_ui_resize(&ui,640,400)&&!nova_files_ui_resize(&ui,639,400)&&!nova_files_ui_resize(&ui,640,399));
 unsigned int sizes[][2]={{640,400},{1280,800},{1600,900},{1920,1080}};
 for(unsigned int t=0;t<3;t++)for(unsigned int n=0;n<4;n++){
  unsigned int w=sizes[n][0],h=sizes[n][1],pitch=w*4+16;size_t bytes=(size_t)pitch*h;unsigned char*p=malloc(bytes+64);assert(p);memset(p,0x61,bytes+64);ui.theme=t;assert(nova_files_ui_resize(&ui,w,h));assert(nova_files_ui_render(&ui,p+32,w,h,pitch));
  for(unsigned int k=0;k<32;k++)assert(p[k]==0x61&&p[bytes+32+k]==0x61);
  for(unsigned int y=0;y<h;y++)for(unsigned int k=w*4;k<pitch;k++)assert(p[32+(size_t)y*pitch+k]==0x61);free(p);
 }
 unsigned int w=960,h=250;unsigned int*p=malloc(w*h*4+64);assert(p);for(unsigned int i=0;i<w*h+16;i++)p[i]=0x00f6f8fa;
 struct nova_ui_canvas c={(unsigned char*)p,w,h,w*4};nova_ui_text(c,24,38,3,1,0x263040,"R19 Inter: actual proportional advances / Turkish / separate 2x atlas",0,0,w,h);
 nova_ui_text(c,24,82,0,1,0x263040,tr,0,0,w,h);nova_ui_text(c,24,146,0,2,0x263040,tr,0,0,w,h);
 for(unsigned int i=0;i<5;i++)nova_ui_app_icon(c,24+(int)i*66,178,48,i);for(unsigned int i=0;i<16;i++)assert(p[w*h+i]==0x00f6f8fa);ppm(argv[1],p,w,h);free(p);
 nova_files_close(&model);puts("R19_PROPORTIONAL_BASELINE_TURKISH_UTF8_ELLIPSIS_BOUNDS=PASS");puts("R19_SCOPED_QUICK_LOCATION_TRAVERSAL_DENIAL_THEMES_COMPACT_LAYOUT=PASS");puts("R19_FOUR_VIEWPORTS_THREE_THEMES_PITCH_REDZONES_2X_ATLAS_DIAGNOSTIC=PASS (not desktop HiDPI acceptance)");return 0;
}
