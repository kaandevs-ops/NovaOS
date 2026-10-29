#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "nova_files_guest_adapter.h"
int main(void){
 char path[]="/tmp/nova-r16-fs-XXXXXX";assert(mkdtemp(path));
 int fd=open(path,O_RDONLY|O_DIRECTORY);assert(fd>=0);
 assert(mkdirat(fd,"Documents",0700)==0);
 assert(symlinkat("/etc",fd,"escape")==0);
 assert(nova_files_guest_open(fd,"escape",0)<0);
 assert(nova_files_guest_open(fd,"..",0)<0);
 assert(nova_files_guest_open(fd,"/etc",0)<0);
 struct nova_files_model m;
 assert(nova_files_init(&m,fd,&nova_files_guest_ops,0)==0);
 assert(nova_files_list(&m)==0 && m.count==2);
 assert(nova_files_enter(&m,"Documents")==0 && m.depth==1);
 assert(nova_files_parent(&m)==0 && m.depth==0);
 assert(nova_files_parent(&m)==NOVA_FILES_ESCAPE);
 assert(nova_files_enter(&m,"../etc")==NOVA_FILES_ESCAPE);
 nova_files_close(&m);
 printf("R16_GUEST_RAW_SYSCALL_BACKEND=PASS\nR16_OPENAT2_SYMLINK_ABSOLUTE_PARENT_DENIED=PASS\nARTIFACTS=%s\n",path);
 return 0;
}
