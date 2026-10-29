#define _GNU_SOURCE
#include "nova_desktop_store.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
int main(void){char path[]="/tmp/nova-r47-desktop-XXXXXX";assert(mkdtemp(path));
 int h=open(path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(h>=0);
 assert(nova_desktop_create(h,"Notlar.txt",0)==0);
 assert(nova_desktop_create(h,"Ders",1)==0);
 assert(nova_desktop_create(h,"Notlar.txt",0)==-1&&errno==EEXIST);
 assert(nova_desktop_create(h,"../escape",0)==-1);
 assert(nova_desktop_create(h,"/escape",1)==-1);
 assert(nova_desktop_create(h,".hidden",0)==-1);
 assert(nova_desktop_create(h,"bad\\name",0)==-1);
 assert(nova_desktop_create(h,"bad\nname",0)==-1);
 assert(nova_desktop_create(h,"bad",2)==-1);
 struct nova_desktop_snapshot snap;assert(!nova_desktop_action(h,2,&snap));assert(snap.count==3&&!snap.status);assert(!nova_desktop_action(h,3,&snap));assert(snap.count==4);assert(nova_desktop_action(h,99,&snap)==-1&&snap.status==EINVAL);
 int d=nova_desktop_dir(h);assert(d>=0);struct stat s;
 assert(!fstatat(d,"Notlar.txt",&s,AT_SYMLINK_NOFOLLOW)&&S_ISREG(s.st_mode)&&s.st_size==0&&(s.st_mode&0777)==0600);
 assert(!symlinkat("/tmp",d,"Link"));assert(nova_desktop_create(h,"Link",0)==-1);
 for(unsigned i=0;i<28;i++){char name[32];snprintf(name,sizeof name,"Bound-%02u",i);assert(nova_desktop_create(h,name,0)==0);}assert(!nova_desktop_snapshot(h,&snap)&&snap.count==NOVA_DESKTOP_LIMIT&&snap.clipped);
 struct nova_term_packet req={.magic=NOVA_TERM_MAGIC,.version=1,.op=NOVA_TERM_DESKTOP,.rows=2};assert(nova_term_valid(&req,NOVA_TERM_HEADER,1));req.rows=4;assert(!nova_term_valid(&req,NOVA_TERM_HEADER,1));req.rows=1;req.cols=1;assert(!nova_term_valid(&req,NOVA_TERM_HEADER,1));
 assert(!fchmod(d,0777));assert(nova_desktop_create(h,"Rejected",0)==-1);assert(!fchmod(d,0700));close(d);close(h);
 char second[]="/tmp/nova-r47-link-XXXXXX";assert(mkdtemp(second));h=open(second,O_RDONLY|O_DIRECTORY);assert(h>=0);
 assert(!symlinkat(path,h,"Desktop"));assert(nova_desktop_create(h,"escape",0)==-1);close(h);
 assert(sizeof(struct nova_desktop_snapshot)<=NOVA_TERM_BYTES);puts("PASS desktop real creates, bounded snapshots, ordering, invalid actions, no overwrite, traversal, symlinks and permissions");return 0;}
