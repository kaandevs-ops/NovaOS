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
static long failing_read(int fd,void*p,unsigned long n,void*c){(void)fd;(void)p;(void)n;(void)c;return -1;}
static unsigned repeats;
static long endless_read(int fd,void*p,unsigned long n,void*c){
 (void)fd;(void)c;assert(n>=32);unsigned char*b=p;memset(b,0,32);b[16]=32;b[18]=8;memcpy(b+19,"repeat",7);repeats++;return 32;
}
static void draw(struct nova_files_ui*u,const char*path){
 unsigned w=800,h=500;assert(nova_files_ui_resize(u,w,h));size_t n=(size_t)w*h*4;unsigned char*b=malloc(n+64);assert(b);memset(b,0x6b,n+64);
 assert(nova_files_ui_render(u,b,w,h,w*4));for(unsigned i=0;i<64;i++)assert(b[n+i]==0x6b);
 if(path){FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(size_t i=0;i<(size_t)w*h;i++){fputc(b[i*4+2],f);fputc(b[i*4+1],f);fputc(b[i*4],f);}assert(!fclose(f));}free(b);
}
int main(int argc,char**argv){
 assert(argc==3);char directory[]="/tmp/nova-r40-pages-XXXXXX";assert(mkdtemp(directory));int fd=open(directory,O_RDONLY|O_DIRECTORY);assert(fd>=0);
 for(unsigned i=197;i>0;i--){char name[64];snprintf(name,sizeof name,"item-%03u.txt",i-1);int f=openat(fd,name,O_WRONLY|O_CREAT|O_EXCL,0600);assert(f>=0);assert(write(f,name,strlen(name))==(ssize_t)strlen(name));close(f);}
 for(unsigned i=0;i<5;i++){char name[32];snprintf(name,sizeof name,"Folder-%u",i);assert(!mkdirat(fd,name,0700));}
 struct nova_files_model m;assert(!nova_files_init(&m,fd,&nova_files_guest_ops,0));assert(!nova_files_list(&m));assert(m.count==48&&m.scanned==202&&m.has_next&&!m.has_previous);
 unsigned all=0,pages=0;char previous[NOVA_FILES_NAME_BYTES]={0};unsigned dir_count=0;
 do{
  for(unsigned i=0;i<m.count;i++){
   if(m.entries[i].type==4)dir_count++;
   else if(previous[0])assert(strcmp(previous,m.entries[i].name)<0);
   if(m.entries[i].type==8)strcpy(previous,m.entries[i].name);
   all++;
  }
  pages++;if(!m.has_next)break;assert(!nova_files_page(&m,1));
 }while(pages<10);
 assert(all==202&&pages==5&&dir_count==5&&m.count==10&&m.has_previous&&!m.has_next);
 struct nova_files_model snapshot=m;assert(nova_files_page(&m,1)==NOVA_FILES_FULL&&!memcmp(&m,&snapshot,sizeof m));
 unsigned backward=0;
 do{backward+=m.count;if(!m.has_previous)break;assert(!nova_files_page(&m,0));}while(backward<250);
 /* Last page is partial; reverse pages use the nearest 48 predecessors. */
 assert(backward==202&&!m.has_previous&&m.has_next);
 assert(!nova_files_query(&m,"ITEM-196"));assert(m.count==1&&!strcmp(m.entries[0].name,"item-196.txt")&&m.scanned==202&&!m.has_next&&!m.has_previous);
 assert(!nova_files_query(&m,m.listing_query)&&m.count==1);
 assert(!nova_files_query(&m,"item-"));assert(m.count==48&&m.has_next&&!m.has_previous);assert(!nova_files_page(&m,1)&&!strcmp(m.listing_query,"item-"));
 snapshot=m;m.ops.read_entries=failing_read;struct nova_files_model fail=m;assert(nova_files_page(&m,1)==NOVA_FILES_IO&&!memcmp(&m,&fail,sizeof m));m=snapshot;
 char oversized[NOVA_FILES_QUERY_BYTES+1];memset(oversized,'x',sizeof oversized);oversized[sizeof oversized-1]=0;snapshot=m;assert(nova_files_query(&m,oversized)==NOVA_FILES_BAD&&!memcmp(&m,&snapshot,sizeof m));
 m.ops.read_entries=endless_read;fail=m;assert(nova_files_query(&m,"")==NOVA_FILES_FULL&&repeats==256&&!memcmp(&m,&fail,sizeof m));m=snapshot;
 assert(!nova_files_list(&m));struct nova_files_ui ui;nova_files_ui_init(&ui,&m);draw(&ui,argv[1]);
 assert(nova_files_ui_key(&ui,'n')>0&&m.has_previous&&ui.selected==~0U);assert(nova_files_ui_key(&ui,'p')>0&&!m.has_previous);
 assert(nova_files_ui_find(&ui));for(const char*s="item-196";*s;s++)assert(nova_files_ui_filter_key(&ui,(unsigned char)*s)>0);assert(m.count==1&&nova_files_ui_match_count(&ui)==1);draw(&ui,argv[2]);
 assert(nova_files_ui_pointer(&ui,770,86,1,1)>0&&!ui.name_filter[0]&&m.count==48&&m.has_next);
 for(unsigned size=0;size<3;size++)for(unsigned theme=0;theme<3;theme++){
  unsigned w=size==0?640:size==1?800:1280,h=size==0?400:size==1?500:800;assert(nova_files_ui_resize(&ui,w,h));ui.theme=theme;
  size_t n=(size_t)w*h*4;unsigned char*b=malloc(n+64);assert(b);memset(b,0x4e,n+64);assert(nova_files_ui_render(&ui,b,w,h,w*4));for(unsigned j=0;j<64;j++)assert(b[n+j]==0x4e);free(b);
  int x=(int)nova_files_ui_sidebar_width(w)+190;assert(nova_files_ui_pointer(&ui,x,h-16,1,1)>0&&m.has_previous);assert(nova_files_ui_pointer(&ui,x,h-16,1,0)>0);
  x=(int)nova_files_ui_sidebar_width(w)+154;assert(nova_files_ui_pointer(&ui,x,h-16,1,1)>0&&!m.has_previous);assert(nova_files_ui_pointer(&ui,x,h-16,1,0)>0);
 }
 nova_files_close(&m);puts("R40_REAL_DIRECTORY_202_ENTRIES_FORWARD_BACKWARD_QUERY_BOUNDARY_FAILURE_BUDGET_UI_CLEAR_AND_CLIPPING_PASS");return 0;
}