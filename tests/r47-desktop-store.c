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
 int d=nova_desktop_dir(h);assert(d>=0);struct stat s;
 assert(!fstatat(d,"Notlar.txt",&s,AT_SYMLINK_NOFOLLOW)&&S_ISREG(s.st_mode)&&s.st_size==0&&(s.st_mode&0777)==0600);
 assert(!symlinkat("/tmp",d,"Link"));assert(nova_desktop_create(h,"Link",0)==-1);
 assert(!fchmod(d,0777));assert(nova_desktop_create(h,"Rejected",0)==-1);assert(!fchmod(d,0700));close(d);close(h);
 char second[]="/tmp/nova-r47-link-XXXXXX";assert(mkdtemp(second));h=open(second,O_RDONLY|O_DIRECTORY);assert(h>=0);
 assert(!symlinkat(path,h,"Desktop"));assert(nova_desktop_create(h,"escape",0)==-1);close(h);
 puts("PASS real file/directory creation, no overwrite, traversal, symlinks, permissions; GUI integration pending");return 0;}
