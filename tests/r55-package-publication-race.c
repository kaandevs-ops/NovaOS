#define _GNU_SOURCE
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/syscall.h>
long racing_syscall(long,...);
#define syscall racing_syscall
#define main pkg_main
#include "../src/terminal/nova_pkg.c"
#undef main
#undef syscall
long racing_syscall(long nr,...){
 assert(nr==SYS_renameat2);va_list ap;va_start(ap,nr);int d=va_arg(ap,int);char*old=va_arg(ap,char*);int to=va_arg(ap,int);const char*name=va_arg(ap,const char*);unsigned flags=va_arg(ap,unsigned);va_end(ap);
 int f=openat(to,name,O_WRONLY|O_CREAT|O_EXCL,0600);assert(f>=0);assert(write(f,"RACE_SENTINEL",13)==13);close(f);
 return syscall(nr,d,old,to,name,flags);
}
int main(int argc,char**argv){return pkg_main(argc,argv);}
