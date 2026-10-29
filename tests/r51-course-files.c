#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "nova_files_ui.h"
#include "nova_files_guest_metadata.h"
static int preview(int fd,const char*name,char*out,unsigned cap){int f=openat(fd,name,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if(f<0)return -13;ssize_t n=read(f,out,cap-1);close(f);if(n<0)return -1;out[n]=0;return (int)n;}
static void frame(struct nova_files_ui*u,const char*out,const char*name,unsigned w,unsigned h,int keep){assert(nova_files_ui_resize(u,w,h));size_t n=(size_t)w*h*4;unsigned char*p=malloc(n+128);assert(p);memset(p,0xa5,n+128);assert(nova_files_ui_render(u,p+64,w,h,w*4));for(unsigned i=0;i<64;i++)assert(p[i]==0xa5&&p[n+64+i]==0xa5);if(keep){char path[1024];snprintf(path,sizeof path,"%s/%s.ppm",out,name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);unsigned*pix=(unsigned*)(p+64);for(unsigned i=0;i<w*h;i++){unsigned char rgb[]={pix[i]>>16,pix[i]>>8,pix[i]};assert(fwrite(rgb,1,3,f)==3);}fclose(f);}free(p);}
int main(int argc,char**argv){assert(argc==3&&getuid()==12004);const char*folders[]={"01-pwd-ls","02-ls-la","03-cat","04-grep","05-grep-log","06-find","07-find-gizli","08-ls-stat","09-chmod","10-chown","11-pipe","12-GUI-CLI"};int fd=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW);assert(fd>=0);struct nova_files_model m;assert(!nova_files_init(&m,fd,&nova_files_guest_ops,0));struct nova_files_ui ui;
for(unsigned i=0;i<12;i++){char path[256];snprintf(path,sizeof path,"/Desktop/%s",folders[i]);assert(!nova_files_navigate(&m,path));assert(!nova_files_list(&m));assert(m.count>=4&&!m.truncated);nova_files_ui_init(&ui,&m);nova_files_ui_set_metadata(&ui,nova_files_guest_metadata);ui.workspace_shared=1;for(unsigned j=0;j<m.count;j++){assert(ui.metadata[j].valid&&ui.metadata[j].posix_valid);assert(ui.metadata[j].uid==12004&&ui.metadata[j].gid==12004);}frame(&ui,argv[2],"folder",800,500,0);frame(&ui,argv[2],"narrow",640,400,0);}
assert(!nova_files_navigate(&m,"/Desktop/02-ls-la/Ders/belgeler"));assert(!nova_files_list(&m));nova_files_ui_init(&ui,&m);nova_files_ui_set_metadata(&ui,nova_files_guest_metadata);frame(&ui,argv[2],"02-ls-la-real-files-HOST-NOT-VM",800,500,1);
assert(!nova_files_navigate(&m,"/Desktop/12-GUI-CLI/Week4"));assert(!nova_files_list(&m));nova_files_ui_init(&ui,&m);nova_files_ui_set_metadata(&ui,nova_files_guest_metadata);ui.workspace_shared=1;ui.preview_reader=preview;unsigned found=~0U;for(unsigned i=0;i<m.count;i++)if(!strcmp(m.entries[i].name,"notes.txt"))found=i;assert(found!=~0U);ui.selected=found;assert(nova_files_ui_key(&ui,13));assert(ui.preview_open&&ui.preview_status>0&&strstr(ui.preview_text,"Aynı dosya, iki arayüz"));frame(&ui,argv[2],"12-same-real-notes-preview-HOST-NOT-VM",800,500,1);nova_files_close(&m);puts("PASS 12 actual Files directories, real UID12004/openat2/statx metadata, 24 guarded layouts and actual shared notes preview; HOST NOT VM");return 0;}
