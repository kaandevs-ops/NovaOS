#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "nova_files_guest_metadata.h"
#include "../files/src/nova_files_ui.c"
static unsigned int fd_count(void){DIR*d=opendir("/proc/self/fd");assert(d);unsigned int n=0;while(readdir(d))n++;closedir(d);return n;}
int main(void){char root[]="/tmp/nova-r22-metadata-XXXXXX";assert(mkdtemp(root));int fd=open(root,O_RDONLY|O_DIRECTORY);assert(fd>=0);int f=openat(fd,"Sample.txt",O_RDWR|O_CREAT|O_EXCL,0600);assert(f>=0&&!ftruncate(f,1536));assert(!mkdirat(fd,"Folder",0700)&&!symlinkat("/etc/passwd",fd,"Link")&&!mkfifoat(fd,"Pipe",0600));
 int small=openat(fd,"Small.txt",O_WRONLY|O_CREAT|O_EXCL,0600);assert(small>=0&&!ftruncate(small,64));close(small);
 struct nova_files_metadata m;assert(nova_files_guest_metadata(fd,"Sample.txt",8,&m)&&m.valid&&m.bytes==1536);assert(nova_files_guest_metadata(fd,"Folder",4,&m));
 const char*bad[]={"../etc/passwd","Folder/..",".","..","","Link","Pipe","/etc/passwd","x\\y"};for(unsigned int i=0;i<9;i++)assert(!nova_files_guest_metadata(fd,bad[i],8,&m)&&!m.valid);assert(!nova_files_guest_metadata(fd,"Sample.txt",4,&m));assert(!nova_files_guest_metadata(-1,"Sample.txt",8,&m));
 unsigned int before=fd_count();off_t offset=lseek(fd,0,SEEK_CUR);for(unsigned int i=0;i<1000;i++){assert(nova_files_guest_metadata(fd,"Sample.txt",8,&m));assert(!nova_files_guest_metadata(fd,"Link",8,&m));assert(!nova_files_guest_metadata(fd,"Pipe",8,&m));}assert(fd_count()==before&&lseek(fd,0,SEEK_CUR)==offset);
 char text[64];size_text(1536,text);assert(!strcmp(text,"1.5 KiB"));size_text(0,text);assert(!strcmp(text,"0 B"));size_text(~0ULL,text);assert(!strcmp(text,"15.9 EiB"));
 struct nova_files_model model;assert(!nova_files_init(&model,fd,&nova_files_guest_ops,0)&&!nova_files_list(&model));struct nova_files_ui u;nova_files_ui_init(&u,&model);nova_files_ui_set_metadata(&u,nova_files_guest_metadata);unsigned int i=0;while(i<model.count&&strcmp(model.entries[i].name,"Sample.txt"))i++;assert(i<model.count&&u.metadata[i].valid&&u.metadata[i].bytes==1536);assert(!ftruncate(f,2048)&&nova_files_ui_key(&u,'r')>0&&u.metadata[i].bytes==2048);assert(nova_files_ui_pointer(&u,730,116,1,1)>0&&u.sort_mode==3);assert(nova_files_ui_pointer(&u,730,116,1,1)>0&&u.sort_mode==4);u.sort_mode=0;u.selected=i;assert(nova_files_ui_key(&u,'s')>0&&u.selected==i);assert(nova_files_ui_key(&u,'f')>0&&u.selected==~0U);
 unsigned int sizes[][2]={{640,400},{800,500},{1280,800},{1920,1080}};
 for(unsigned int v=0;v<4;v++)for(unsigned int t=0;t<3;t++){unsigned int w=sizes[v][0],h=sizes[v][1],pitch=w*4+16;unsigned long bytes=(unsigned long)pitch*h;unsigned char*b=malloc(bytes+64);memset(b,0x65,bytes+64);assert(nova_files_ui_resize(&u,w,h));u.theme=t;assert(nova_files_ui_render(&u,b+32,w,h,pitch));for(unsigned int k=0;k<32;k++)assert(b[k]==0x65&&b[bytes+32+k]==0x65);for(unsigned int y=0;y<h;y++)for(unsigned int k=w*4;k<pitch;k++)assert(b[32+(unsigned long)y*pitch+k]==0x65);free(b);}
 u.folders_only=0;u.sort_mode=0;u.selected=i;assert(nova_files_ui_key(&u,'b')>0&&u.sort_mode==3&&u.selected==i);assert(nova_files_ui_key(&u,36)>0&&!strcmp(model.entries[u.selected].name,"Small.txt"));assert(nova_files_ui_key(&u,'b')>0&&u.sort_mode==4);assert(nova_files_ui_key(&u,36)>0&&!strcmp(model.entries[u.selected].name,"Sample.txt"));
 model.truncated=1;model.scanned=10000;status_text(&u,1,text);assert(!strcmp(text,"1 item / 10000 scanned"));
 close(f);nova_files_close(&model);puts("R22_REAL_FD_METADATA_SIZE_TYPE_TRAVERSAL_SYMLINK_SPECIAL_1000_FD_CYCLES_REFRESH_VIEW_BOUNDS=PASS");return 0;}
