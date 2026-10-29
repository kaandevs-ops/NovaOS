#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../files/src/nova_files_ui.c"
extern const struct nova_files_ops nova_files_host_ops;
static int metadata(int fd,const char*n,unsigned char type,struct nova_files_metadata*m){struct stat s;m->valid=0;m->bytes=0;if(fstatat(fd,n,&s,AT_SYMLINK_NOFOLLOW))return 0;if((type==8&&!S_ISREG(s.st_mode))||(type==4&&!S_ISDIR(s.st_mode)))return 0;m->valid=1;m->bytes=s.st_size;return 1;}
static unsigned find(struct nova_files_model*m,const char*n){for(unsigned i=0;i<m->count;i++)if(!strcmp(m->entries[i].name,n))return i;return ~0U;}
static void put(int dir,const char*n,const char*s){int fd=openat(dir,n,O_CREAT|O_TRUNC|O_WRONLY,0600);assert(fd>=0);assert(write(fd,s,strlen(s))==(ssize_t)strlen(s));assert(!close(fd));}
int main(int argc,char**argv){assert(argc==2);int root=open(argv[1],O_DIRECTORY|O_RDONLY);assert(root>=0);put(root,"middle.txt","one");put(root,"zeta.txt","z");struct nova_files_model m;struct nova_files_ui u;assert(!nova_files_init(&m,root,&nova_files_host_ops,0)&&!nova_files_list(&m));nova_files_ui_init(&u,&m);nova_files_ui_set_metadata(&u,metadata);assert(!u.workspace_shared);u.workspace_shared=1;u.selected=find(&m,"middle.txt");assert(u.selected!=~0U);u.properties_open=1;
put(root,"aaa.txt","before");assert(nova_files_ui_key(&u,'r')==1);assert(u.selected==find(&m,"middle.txt")&&u.properties_open);
assert(!nova_files_ui_poll_metadata(&u));put(root,"zeta.txt","updated size");assert(nova_files_ui_poll_metadata(&u)==1);assert(u.metadata[find(&m,"zeta.txt")].bytes==12);assert(!nova_files_ui_poll_metadata(&u));
assert(!unlinkat(root,"middle.txt",0));assert(nova_files_ui_key(&u,'r')==1&&u.selected==~0U&&!u.properties_open);
assert(!mkdirat(root,"Documents",0700));assert(nova_files_ui_key(&u,'r')==1);assert(u.place_count==1&&!strcmp(u.places[0],"Documents"));
unsigned w=800,h=500;unsigned char *guard=malloc(w*h*4+128);assert(guard);memset(guard,0xa5,w*h*4+128);assert(nova_files_ui_render(&u,guard+64,w,h,w*4));for(unsigned i=0;i<64;i++)assert(guard[i]==0xa5&&guard[w*h*4+64+i]==0xa5);free(guard);nova_files_close(&m);puts("PASS: refresh preserves filename selection; deletion clears properties; visible metadata change triggers one redraw; unchanged poll triggers none; sidebar refresh; shared view guards");}
