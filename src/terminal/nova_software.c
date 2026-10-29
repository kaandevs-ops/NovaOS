#define _GNU_SOURCE
/* Real terminal application manager. No shell strings, downloads or privilege changes. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include "../runtime/nova_local_catalog.h"
static struct termios original;
static volatile sig_atomic_t stopping,resized;
static void resize_notice(int sig){(void)sig;resized=1;}
static int fits(void){struct winsize w;return !ioctl(0,TIOCGWINSZ,&w)&&w.ws_col>=60&&w.ws_row>=20;}
static void stop(int sig){(void)sig;stopping=1;}
static int raw(void){struct termios t=original;t.c_lflag&=~(ICANON|ECHO);t.c_cc[VMIN]=1;t.c_cc[VTIME]=0;return tcsetattr(0,TCSANOW,&t);}
static int input(void){unsigned char c;for(;;){if(stopping)return -1;if(resized)return -2;ssize_t n=read(0,&c,1);if(n==1)return c;if(n<0&&errno==EINTR&&!stopping)continue;return -1;}}
static int action(const char*op,const char*id){
 if(tcsetattr(0,TCSANOW,&original))return -1;
 pid_t p=fork();if(p==0){char*args[]={"nova-pkg",(char*)op,(char*)id,NULL};execv("/bin/nova-pkg",args);perror("nova-software exec");_exit(127);}
 if(p<0){raw();return -1;}int s;pid_t got;do{got=waitpid(p,&s,0);}while(got<0&&errno==EINTR);
 int result=got==p&&WIFEXITED(s)?WEXITSTATUS(s):128;if(raw())return -1;return result;
}
static int package_state(const char*id){
 pid_t p=fork();if(!p){close(1);close(2);char*args[]={"nova-pkg","status",(char*)id,"--machine",NULL};execv("/bin/nova-pkg",args);_exit(127);}
 if(p<0)return 1;int s;pid_t got;do{got=waitpid(p,&s,0);}while(got<0&&errno==EINTR);
 return got==p&&WIFEXITED(s)?WEXITSTATUS(s):1;
}
static const char*state_label(int state){return state==0?"kurulu / dogrulandi":state==3?"kurulu degil":"dogrulama hatasi";}
int main(int argc,char**argv){
 (void)argv;if(argc!=1||getuid()!=12004||geteuid()!=12004){fputs("nova-software: terminal user required\n",stderr);return 2;}
 if(!isatty(0)||tcgetattr(0,&original)||raw()){fputs("nova-software: interactive terminal required\n",stderr);return 1;}
 struct sigaction a={0};a.sa_handler=stop;sigemptyset(&a.sa_mask);sigaction(SIGINT,&a,NULL);sigaction(SIGTERM,&a,NULL);a.sa_handler=resize_notice;sigaction(SIGWINCH,&a,NULL);
 unsigned selected=0;int result=0,redraw=1,compact=0;const unsigned count=sizeof(nova_catalog)/sizeof(nova_catalog[0]);
 while(!stopping){
  if(resized){resized=0;redraw=1;}
  if(redraw){compact=!fits();if(compact){printf("\033[2J\033[HNova Software: window too small.\nNeeds 60 columns and 20 rows; maximize terminal.\nq: quit\n");fflush(stdout);redraw=0;}else{printf("\033[2J\033[HNova Software / Yerel uygulamalar\n\n");
   for(unsigned i=0;i<count;i++)printf("%c %u. %s %s - %s\n",i==selected?'>':' ',i+1,nova_catalog[i].title,nova_catalog[i].version,state_label(package_state(nova_catalog[i].id)));
   const struct nova_catalog_entry*p=&nova_catalog[selected];printf("\n%s\n\n1-%u: sec | i: kur | o: calistir | s: durum\nu: yerel guncelle | r: kaldir | v: yenile | q: cik\n\nYalniz dogrulanmis yerel katalog. Internet deposu yok.\nUygulamalar terminal icinde ve mevcut sandbox ile calisir.\n\n",p->summary,count);fflush(stdout);redraw=0;
  }}
  int c=input();if(c==-2)continue;if(c<0)break;if(c=='q'||c=='Q')break;if(compact)continue;
  if(c>='1'&&(unsigned)(c-'1')<count){selected=(unsigned)(c-'1');redraw=1;continue;}
  if(c=='v'||c=='V'){redraw=1;continue;}
  if(c==27){struct pollfd poller={.fd=0,.events=POLLIN};int b=poll(&poller,1,100)>0?input():-1;int d=b=='['&&poll(&poller,1,100)>0?input():-1;if(d=='A'){selected=selected?selected-1:count-1;redraw=1;}else if(d=='B'){selected=(selected+1)%count;redraw=1;}continue;}
  if(c=='q'||c=='Q')break;
  const char*op=c=='i'||c=='I'?"install":c=='o'||c=='O'?"run":c=='s'||c=='S'?"status":c=='u'||c=='U'?"upgrade":c=='r'||c=='R'?"remove":NULL;
  if(!op)continue;
  if(!strcmp(op,"remove")){printf("\nKaldir? y: onay, diger: iptal\n");fflush(stdout);if(input()!='y'){redraw=1;continue;}}
  int rc=action(op,nova_catalog[selected].id);if(rc<0){result=1;break;}
  if(!strcmp(op,"run")){redraw=1;continue;}
  printf("\nIslem sonucu: %d (0 = basarili)\nBir tusa basin...\n",rc);fflush(stdout);if(input()<0)break;redraw=1;
 }
 if(tcsetattr(0,TCSANOW,&original))result=1;printf("\033[0m\nNova Software kapandi.\n");return result;
}
