#include "../src/terminal/nova_pty_host.h"
#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
static int fd_count(void) { DIR *d=opendir("/proc/self/fd");assert(d);int n=0;while(readdir(d))n++;closedir(d);return n; }
int main(void) {
 int before=fd_count();
 struct nova_pty p;char *bad[]={"missing",NULL};
 assert(nova_pty_start(&p,"relative",bad,24,80)<0);
 assert(nova_pty_start(&p,"/no/nova-executable",bad,24,80)<0 && errno==ENOENT);
 for(int cycle=0;cycle<20;cycle++) {
 char *args[]={"/bin/sh","-c","read value; printf 'NOVA_REAL:%s\\n' \"$value\"; stty size",NULL};
 assert(nova_pty_start(&p,"/bin/sh",args,24,80)==0);
 assert(nova_pty_resize(&p,0,80)<0);
 assert(nova_pty_resize(&p,37,101)==0);
 struct winsize w;assert(ioctl(p.master,TIOCGWINSZ,&w)==0 && w.ws_row==37 && w.ws_col==101);
 assert(write(p.master,"hello\n",6)==6);
 char out[4096]={0};size_t used=0;
 for(int i=0;i<100 && !strstr(out,"37 101");i++) {
 struct pollfd f={p.master,POLLIN,0};if(poll(&f,1,50)>0) {ssize_t n=read(p.master,out+used,sizeof(out)-used-1);if(n>0)used+=(size_t)n;}
 }
 assert(strstr(out,"NOVA_REAL:hello"));assert(strstr(out,"37 101"));
 assert(nova_pty_close(&p,NULL)==0);assert(p.master==-1 && p.child==-1);
 }
 assert(fd_count()==before);
 puts("PASS: real shell PTY input/output, resize, exec failure, 20 lifecycles (HOST ONLY)");
}
