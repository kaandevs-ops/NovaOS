#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "nova_files_guest_metadata.h"
#include "nova_files_ui.c"
static void render(struct nova_files_ui*u,const char*path){
 unsigned w=u->viewport_width,h=u->viewport_height,pitch=w*4+16;size_t bytes=(size_t)pitch*h;unsigned char*b=malloc(bytes+64);assert(b);memset(b,0x79,bytes+64);assert(nova_files_ui_render(u,b+32,w,h,pitch));
 for(unsigned k=0;k<32;k++)assert(b[k]==0x79&&b[32+bytes+k]==0x79);
 for(unsigned y=0;y<h;y++)for(unsigned k=w*4;k<pitch;k++)assert(b[32+(size_t)y*pitch+k]==0x79);
 if(path){FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){unsigned char*p=b+32+(size_t)y*pitch+x*4;fputc(p[2],f);fputc(p[1],f);fputc(p[0],f);}assert(!fclose(f));}free(b);
}
int main(int argc,char**argv){
 assert(argc==4&&getuid()==0);char path[]="/tmp/nova-r41-posix-ui-XXXXXX";assert(mkdtemp(path));int root=open(path,O_RDONLY|O_DIRECTORY);assert(root>=0);int f=openat(root,"Permission-demo.txt",O_RDWR|O_CREAT|O_EXCL,0600);assert(f>=0&&write(f,"Actual file content\n",20)==20);assert(!fchown(f,12004,12004));
 struct nova_files_model m;assert(!nova_files_init(&m,root,&nova_files_guest_ops,0)&&!nova_files_list(&m));struct nova_files_ui u;nova_files_ui_init(&u,&m);nova_files_ui_set_metadata(&u,nova_files_guest_metadata);u.selected=0;assert(nova_files_ui_key(&u,'i')==1&&u.properties_open);assert(u.metadata[0].valid&&u.metadata[0].bytes==20);u.theme=2;
#ifdef NOVA_R41_POSIX
 assert(u.metadata[0].posix_valid&&u.metadata[0].mode==0600&&u.metadata[0].uid==12004&&u.metadata[0].gid==12004);
 const unsigned modes[]={0,0600,0644,04755,02710,01000,01777};const char*expect[]={"---------","rw-------","rw-r--r--","rwsr-xr-x","rwx--s---","--------T","rwxrwxrwt"};
 for(unsigned i=0;i<7;i++){char bits[10],octal[5];nova_ui_permissions(modes[i],bits);nova_ui_mode_octal(modes[i],octal);assert(!strcmp(bits,expect[i])&&strtoul(octal,0,8)==modes[i]);}
 char id[11];nova_ui_unsigned(~0U,id);assert(!strcmp(id,"4294967295"));
#endif
 render(&u,argv[1]);assert(!fchmod(f,0));
#ifdef NOVA_R41_POSIX
 assert(nova_files_ui_poll_metadata(&u)==1&&u.metadata[0].posix_valid&&u.metadata[0].mode==0&&u.selected==0);assert(nova_files_ui_poll_metadata(&u)==0);
#else
 (void)nova_files_ui_poll_metadata(&u);
#endif
 render(&u,argv[2]);assert(!fchmod(f,0640)&&!fchown(f,12006,12006));
#ifdef NOVA_R41_POSIX
 assert(nova_files_ui_poll_metadata(&u)==1&&u.metadata[0].mode==0640&&u.metadata[0].uid==12006&&u.metadata[0].gid==12006);
#endif
 render(&u,argv[3]);unsigned dims[][2]={{640,400},{800,500},{1280,800},{1920,1080}};
 for(unsigned i=0;i<4;i++)for(unsigned theme=0;theme<3;theme++){assert(nova_files_ui_resize(&u,dims[i][0],dims[i][1]));u.theme=theme;render(&u,0);}
#ifdef NOVA_R41_POSIX
 /* Selected details stay live after the row is scrolled off-screen. */
 for(unsigned i=0;i<12;i++){char name[32];snprintf(name,sizeof name,"zz-%02u.txt",i);int extra=openat(root,name,O_WRONLY|O_CREAT|O_EXCL,0600);assert(extra>=0);close(extra);}
 assert(nova_files_ui_key(&u,'r')==1&&u.selected==0);assert(nova_files_ui_resize(&u,640,400));u.scroll=7;
 assert(!fchmod(f,0644)&&nova_files_ui_poll_metadata(&u)==1&&u.metadata[0].mode==0644&&u.selected==0);render(&u,0);
 /* Loss of metadata must clear prior owner/mode and display Unavailable. */
 assert(!renameat(root,"Permission-demo.txt",root,"retained.txt"));assert(nova_files_ui_poll_metadata(&u)==1&&!u.metadata[0].valid&&!u.metadata[0].posix_valid);render(&u,0);
#endif
 close(f);nova_files_close(&m);puts("PASS real statx, chmod/chown UI refresh, special modes, full UID width, unavailable clearing and 12 theme/size guarded renders");
}
