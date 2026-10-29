#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include "nova_files_ui.h"
#include "nova_files_guest_adapter.h"
#include "nova_files_surface_bridge.h"
#include "nova_input.h"
#include "nova_ui.h"
static int fail_child,failed_fd=-1;
static long seek_op(int fd,void*c){return nova_files_guest_seek(fd,c);}
static long read_op(int fd,void*b,unsigned long n,void*c){return fd==failed_fd?-1:nova_files_guest_read(fd,b,n,c);}
static long open_op(int fd,const char*n,void*c){long result=nova_files_guest_open(fd,n,c);if(fail_child&&result>=0)failed_fd=(int)result;return result;}
static long close_op(int fd,void*c){return nova_files_guest_close(fd,c);}
static int fdcount(void){DIR*d=opendir("/proc/self/fd");assert(d);int n=0;while(readdir(d))n++;closedir(d);return n;}
int main(void){
 char root[]="/tmp/nova-r20-navigation-XXXXXX";assert(mkdtemp(root));int fd=open(root,O_RDONLY|O_DIRECTORY);assert(fd>=0);assert(!mkdirat(fd,"Documents",0700));assert(!mkdirat(fd,"Projects",0700));int docs=openat(fd,"Documents",O_RDONLY|O_DIRECTORY);assert(docs>=0);assert(!mkdirat(docs,"Child",0700));close(docs);assert(!symlinkat("/etc",fd,"escape"));
 for(int i=0;i<40;i++){char name[32];snprintf(name,sizeof(name),"File-%02d.txt",i);int f=openat(fd,name,O_CREAT|O_WRONLY|O_EXCL,0600);assert(f>=0);close(f);}
 struct nova_files_ops ops={seek_op,read_op,open_op,close_op};struct nova_files_model m;assert(!nova_files_init(&m,fd,&ops,0)&&!nova_files_list(&m));int base=fdcount();
 for(int n=0;n<100;n++){assert(!nova_files_navigate(&m,"/Documents/Child")&&m.depth==2);assert(!nova_files_navigate(&m,"/")&&!m.depth);}assert(fdcount()==base);
 assert(!nova_files_navigate(&m,"/Documents"));struct nova_files_model before=m;
 const char*bad[]={"../etc","/../etc","/Documents/../Projects","//Projects","/Projects/","/./Projects","/escape","/Documents/Child/nope"};
 for(unsigned int i=0;i<sizeof(bad)/sizeof(bad[0]);i++){int n=fdcount();assert(nova_files_navigate(&m,bad[i])<0);assert(!memcmp(&m,&before,sizeof(m))&&fdcount()==n);}
 fail_child=1;assert(nova_files_navigate(&m,"/Projects")<0);assert(!memcmp(&m,&before,sizeof(m)));fail_child=0;failed_fd=-1;assert(!nova_files_navigate(&m,"/"));
 struct nova_files_ui ui;nova_files_ui_init(&ui,&m);assert(ui.history_count==1);assert(nova_files_ui_pointer(&ui,70,200,1,1)>0&&!strcmp(m.path,"/Documents"));
 assert(nova_files_ui_key(&ui,13)>0&&!strcmp(m.path,"/Documents/Child"));assert(ui.history_count==3);
 assert(nova_files_ui_pointer(&ui,196+34,87,1,1)>0&&!strcmp(m.path,"/Documents"));assert(nova_files_ui_pointer(&ui,196+34,87,1,0)>0);
 assert(nova_files_ui_pointer(&ui,196+70,87,1,1)>0&&!strcmp(m.path,"/Documents/Child"));assert(nova_files_ui_pointer(&ui,196+70,87,1,0)>0);
 assert(nova_files_ui_key(&ui,'[')>0&&!strcmp(m.path,"/Documents"));assert(nova_files_ui_key(&ui,']')>0&&!strcmp(m.path,"/Documents/Child"));
 assert(nova_files_ui_key(&ui,8)>0&&!strcmp(m.path,"/Documents"));assert(nova_files_ui_key(&ui,'[')>0&&!strcmp(m.path,"/Documents/Child"));
 fail_child=1;unsigned int index=ui.history_index,count=ui.history_count;before=m;assert(nova_files_ui_key(&ui,'[')<0);assert(index==ui.history_index&&count==ui.history_count&&!memcmp(&m,&before,sizeof(m)));fail_child=0;failed_fd=-1;
 /* Click root breadcrumb using the same production layout. */
 assert(nova_files_ui_pointer(&ui,196+190,87,1,1)>0&&!m.depth);
 assert(nova_files_ui_key(&ui,']')==0);/* A new branch discarded forward history. */
 assert(nova_files_ui_pointer(&ui,788,400,1,1)>0&&ui.scroll>0);assert(nova_files_ui_scroll(&ui,INT_MAX)>0&&!ui.scroll);
 assert(nova_files_ui_scroll(&ui,INT_MIN)>0);assert(ui.scroll>0);assert(nova_files_ui_scroll(&ui,INT_MAX)>0&&!ui.scroll);
 assert(nova_files_ui_key(&ui,35)>0&&ui.selected==m.count-1);assert(nova_files_ui_key(&ui,36)>0&&!ui.selected&&!ui.scroll);
 for(int n=0;n<40;n++){assert(nova_files_ui_pointer(&ui,70,200,1,1)>0);assert(nova_files_ui_key(&ui,8)>0);}assert(ui.history_count==16&&ui.history_index==15);
 struct nova_files_surface_bridge bridge;nova_files_surface_bridge_init(&bridge,&ui,3,300,100,80);
 struct nova_surface_event event={0};event.magic=NOVA_SURFACE_PROTOCOL_MAGIC;event.version=NOVA_SURFACE_PROTOCOL_VERSION;event.size=sizeof(event);event.client_id=3;event.surface_id=300;event.type=NOVA_SURFACE_EVENT_POINTER;event.code=NOVA_REL_WHEEL;event.value=-1;event.cursor_x=500;event.cursor_y=300;
 assert(nova_files_surface_bridge_event(&bridge,&event)>0&&ui.scroll==3);event.cursor_x=50;unsigned int scroll=ui.scroll;nova_files_surface_bridge_event(&bridge,&event);assert(ui.scroll==scroll);event.client_id=2;assert(nova_files_surface_bridge_event(&bridge,&event)==-2);
 unsigned char aa[40*32*4]={0},bb[40*32*4]={0};struct nova_ui_canvas ca={aa,40,32,160},cb={bb,40,32,160};struct nova_ui_palette pal=nova_ui_palette_for(2);
 assert(nova_ui_button(ca,0,0,32,30,pal,NOVA_UI_DISABLED|NOVA_UI_HOVER|NOVA_UI_PRESSED)==pal.muted);nova_ui_button(cb,0,0,32,30,pal,NOVA_UI_DISABLED);assert(!memcmp(aa,bb,sizeof(aa)));
 nova_files_close(&m);assert(fdcount()==base-1);puts("R20_ATOMIC_PATH_HISTORY_BREADCRUMB_WHEEL_KEYS_FD_LEAK_NEGATIVE_SECURITY=PASS");return 0;
}
