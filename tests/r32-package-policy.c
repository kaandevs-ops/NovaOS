#define _GNU_SOURCE
#include "../src/terminal/nova_terminal_policy.h"
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){perror(#x);return 1;}} while(0)
int main(void){
 char path[]="/tmp/nova-policy-XXXXXX";int f=mkstemp(path);CHECK(f>=0);
 CHECK(write(f,"hello",5)==5);int ro=open(path,O_RDONLY);CHECK(ro>=0);CHECK(unlink(path)==0);
 CHECK(term_install_policy()==0);
 CHECK(pwrite(f,"A",1,1)==1);char b[6]={0};CHECK(pread(f,b,5,0)==5);CHECK(!strcmp(b,"hAllo"));
 errno=0;CHECK(pwrite(ro,"x",1,0)==-1 && errno==EBADF);
 errno=0;CHECK(socket(AF_INET,SOCK_STREAM,0)==-1 && errno==EPERM);
 errno=0;CHECK(mount("none","/","tmpfs",0,0)==-1 && errno==EPERM);
 errno=0;CHECK(setuid(0)==-1 && errno==EPERM);
 errno=0;CHECK(unshare(CLONE_NEWUSER)==-1 && errno==EPERM);
 errno=0;CHECK(syscall(SYS_memfd_create,"denied",0)==-1 && errno==EPERM);
 int image=(int)syscall(SYS_memfd_create,"verified",0x13);CHECK(image>=0);
 CHECK(write(image,"test",4)==4);CHECK(fcntl(image,F_ADD_SEALS,F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL)==0);
 errno=0;CHECK(write(image,"x",1)==-1 && errno==EPERM);close(image);
 close(ro);close(f);puts("POSITIONAL_IO_AND_DENIALS_PASS");return 0;
}
