#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "../src/runtime/nova_terminal_grid.h"
#include "../src/runtime/nova_ui.h"
#include "../src/terminal/nova_terminal_policy.h"
static void policy_test(void){
 pid_t child=fork();assert(child>=0);
 if(!child){
  assert(term_install_policy()==0);
  errno=0;assert(socket(AF_INET,SOCK_STREAM,0)==-1&&errno==EPERM);
  errno=0;assert(unshare(CLONE_NEWUSER)==-1&&errno==EPERM);
  errno=0;assert(mount(NULL,"/nonexistent-nova-test",NULL,0,NULL)==-1&&errno==EPERM);
  errno=0;assert(syscall(SYS_ptrace,0,0,0,0)==-1&&errno==EPERM);
  errno=0;assert(syscall(SYS_ioctl,0,0x5412,0)==-1&&errno==EPERM); /* TIOCSTI */
  errno=0;assert(syscall(SYS_getpriority,0,0)==-1&&errno==EPERM); /* default deny */
  struct rlimit old,r={32,32};assert(prlimit(0,RLIMIT_NOFILE,NULL,&old)==0);
  errno=0;assert(prlimit(0,RLIMIT_NOFILE,&r,NULL)==-1&&errno==EPERM);
  errno=0;long n=syscall(SYS_clone,(unsigned long)CLONE_NEWNS|SIGCHLD,0,0,0,0);
  if(n==0)_exit(8);assert(n==-1&&errno==EPERM);
  pid_t allowed=fork();assert(allowed>=0);if(!allowed)_exit(0);int st;assert(waitpid(allowed,&st,0)==allowed&&WIFEXITED(st)&&WEXITSTATUS(st)==0);
  _exit(0);
 }
 int st;assert(waitpid(child,&st,0)==child&&WIFEXITED(st)&&WEXITSTATUS(st)==0);
}
int main(void){
 struct {unsigned long before;struct nova_term_grid g;unsigned long after;} guarded={.before=0x1122334455667788UL,.after=0xaabbccddeeff1234UL};
 struct nova_term_grid *g=&guarded.g;nova_term_grid_init(g,24,80);
 const unsigned char text[]="hello\r\nTurkce: \xc4\xb0\xc5\x9f\xc4\xb1\xc4\x9f";
 for(unsigned i=0;i<sizeof text-1;i++)nova_term_feed(g,text+i,1);
 assert(g->cells[0][0]=='h'&&g->cells[1][8]==0x130&&g->cells[1][9]==0x15f);
 unsigned char osc[]={27,']','5','2',';','c',';','x',7,'Z'};nova_term_feed(g,osc,sizeof osc);assert(g->cells[1][12]=='Z');
 unsigned char malformed[]={0xc0,0xaf,0xed,0xa0,0x80};nova_term_feed(g,malformed,sizeof malformed);assert(g->utf_left==0);
 for(unsigned i=0;i<400;i++)nova_term_feed(g,(const unsigned char*)"line\r\n",6);
 assert(g->history_count==128&&g->y<g->rows);
 g->scroll=128;assert(nova_term_view_row(g,0)!=NULL);
 unsigned seed=23;unsigned char bytes[127];
 for(unsigned k=0;k<2000;k++){
  for(unsigned i=0;i<sizeof bytes;i++){seed=seed*1664525U+1013904223U;bytes[i]=(unsigned char)(seed>>24);}
  nova_term_feed(g,bytes,sizeof bytes);nova_term_grid_resize(g,4+k%45,20+k%101);
  assert(g->x<g->cols&&g->y<g->rows&&g->param_count<6&&g->utf_left<=3);
  assert(guarded.before==0x1122334455667788UL&&guarded.after==0xaabbccddeeff1234UL);
 }
 assert(nova_term_key_ascii(1,0,0)==27);
 assert(nova_term_key_ascii(46,0,1)==3&&nova_term_key_ascii(43,1,0)=='|'&&nova_term_key_ascii(28,0,0)=='\r');
 struct nova_term_packet p={.magic=NOVA_TERM_MAGIC,.version=1,.op=NOVA_TERM_INPUT,.length=1};
 assert(nova_term_valid(&p,33,1));p.reserved=1;assert(!nova_term_valid(&p,33,1));p.reserved=0;p.length=2049;assert(!nova_term_valid(&p,2081,1));p.length=0;p.op=NOVA_TERM_RESIZE;p.rows=0;p.cols=80;assert(!nova_term_valid(&p,32,1));
 p=(struct nova_term_packet){.magic=NOVA_TERM_MAGIC,.version=NOVA_TERM_VERSION,.op=NOVA_TERM_READY,.rows=24,.cols=80,.code=NOVA_TERM_UID|NOVA_TERM_PERSISTENT};
 assert(nova_term_valid(&p,NOVA_TERM_HEADER,0));p.code|=0x20000;assert(!nova_term_valid(&p,NOVA_TERM_HEADER,0));
 unsigned pixel=0;struct nova_ui_canvas canvas={(unsigned char*)&pixel,1,1,4};
 for(unsigned i=0;i<100000;i++){seed=seed*1664525U+1013904223U;unsigned base=seed;seed=seed*1664525U+1013904223U;unsigned ink=seed;unsigned alpha=i%256,inv=255-alpha;unsigned expected=alpha?((((base>>16&255)*inv+(ink>>16&255)*alpha+127)/255)<<16)|((((base>>8&255)*inv+(ink>>8&255)*alpha+127)/255)<<8)|((base&255)*inv+(ink&255)*alpha+127)/255:base;pixel=base;nova_ui_pixel(canvas,0,0,ink,alpha);assert(pixel==expected);}
 policy_test();puts("PASS: terminal protocol, UTF-8/grid guards/fuzz, history, keyboard, kernel seccomp negative tests and real fork");
}
