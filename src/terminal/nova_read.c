#define _GNU_SOURCE
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "nova_read_model.h"
#ifndef NOVA_READ_HOME
#define NOVA_READ_HOME "/home/nova"
#endif
static volatile sig_atomic_t stopped,resized;
static void signal_stop(int n){stopped=n;}
static void signal_resize(int n){(void)n;resized=1;}
static void geometry(unsigned*rows,unsigned*cols){
 struct winsize w={0};ioctl(1,TIOCGWINSZ,&w);*rows=w.ws_row?w.ws_row:24;*cols=w.ws_col?w.ws_col:80;
 if(*rows>48)*rows=48;if(*cols>200)*cols=200;
}
static int input(void){
 struct pollfd p={0,POLLIN,0};unsigned char c;
 while(!stopped){if(resized){resized=0;return -2;}int n=poll(&p,1,150);if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)continue;if(p.revents&(POLLHUP|POLLERR|POLLNVAL))return -1;if(read(0,&c,1)==1)return c;}return -1;
}
static int arrow(void){
 unsigned char s[5];unsigned n=0;struct pollfd p={0,POLLIN,0};
 while(n<sizeof s&&poll(&p,1,60)>0&&read(0,s+n,1)==1){n++;if(n>1&&s[n-1]>=0x40&&s[n-1]<=0x7e)break;}
 if(n==2&&s[0]=='[')return s[1]=='A'?1:s[1]=='B'?2:s[1]=='D'?3:s[1]=='C'?4:s[1]=='H'?5:s[1]=='F'?6:0;
 if(n==3&&s[0]=='['&&s[2]=='~')return s[1]=='1'||s[1]=='7'?5:s[1]=='4'||s[1]=='8'?6:s[1]=='5'?3:s[1]=='6'?4:0;return 0;
}
static void clipped(const char*s,unsigned cols){size_t at=0,used=0,n=strlen(s);while(at<n&&used<cols){unsigned k=nova_text_unit((const unsigned char*)s+at,n-at);if(!k)break;fwrite(s+at,1,k,stdout);at+=k;used++;}}
static void draw(struct nova_read_model*d,const char*name,const char*status){
 unsigned rows,cols;geometry(&rows,&cols);printf("\033[2J\033[H");
 if(rows<6||cols<20){clipped("Enlarge window; q exits",cols?cols-1:0);fflush(stdout);return;}
 unsigned view=rows-4;if(d->width!=cols-1)nova_read_reflow(d,cols-1,view);else nova_read_move(d,0,view);
 char line[256];snprintf(line,sizeof line,"Nova Read | %s",name);clipped(line,cols-1);printf("\r\n");
 clipped(cols<40?"q: close | space/b page":cols<60?"space/b page | / find | q close":"Read-only | arrows: line/page | space/b: page | Home/End | /: search | q: close",cols-1);
 for(unsigned row=0;row<view;row++){
  printf("\033[%u;1H",row+3);unsigned index=d->top+row;if(index>=d->count)continue;
  size_t a=d->line[index],b=index+1<d->count?d->line[index+1]:d->length;while(b>a&&d->data[b-1]=='\n')b--;
  unsigned cells=0;while(a<b&&cells++<cols-1){unsigned k=nova_text_unit(d->data+a,b-a);if(!k)break;if(d->data[a]=='\t')putchar('>');else fwrite(d->data+a,1,k,stdout);a+=k;}
 }
 if(cols<60)snprintf(line,sizeof line,"%u-%u/%u | %zu B",d->top+1,d->top+view>d->count?d->count:d->top+view,d->count,d->source_bytes);
 else snprintf(line,sizeof line,"Lines %u-%u / %u | %zu bytes | snapshot",d->top+1,d->top+view>d->count?d->count:d->top+view,d->count,d->source_bytes);
 printf("\033[%u;1H",rows-1);clipped(line,cols-1);printf("\033[%u;1H",rows);clipped(cols<60&&!strcmp(status,"UTF-8 text; tabs shown as >. No files are changed.")?"Read-only snapshot; tabs = >":status,cols-1);fflush(stdout);
}
static int query(char out[81]){
 size_t used=0;out[0]=0;
 for(;;){unsigned rows,cols;geometry(&rows,&cols);printf("\033[%u;1H\033[K/",rows);clipped(out,cols>2?cols-2:0);fflush(stdout);int c=input();if(c==-2)continue;if(c<0)return -1;if(c==27)return 0;if(c==13||c==10)return used?1:0;if(c==127||c==8){if(used)out[--used]=0;}else if(c>=32&&c<127&&used<80){out[used++]=(char)c;out[used]=0;}}
}
int main(int argc,char**argv){
 if(argc!=2||!argv[1][0]){fprintf(stderr,"Usage: nova-read PATH\nRead-only UTF-8 text, up to 64 KiB; relative/current-directory or absolute home paths; no symlinks or .. components. Not PDF/HTML.\n");return 2;}
 if(!isatty(0)||!isatty(1)){fprintf(stderr,"nova-read needs a real interactive terminal.\n");return 2;}
 struct nova_read_model*d=calloc(1,sizeof *d);if(!d){perror("document memory");return 1;}
 int root=open(NOVA_READ_HOME,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
 if(root<0||nova_read_path(root,NOVA_READ_HOME,argv[1],d)){int error=errno;if(root>=0)close(root);free(d);errno=error;perror("read document");return 1;}close(root);
 struct termios saved,raw;if(tcgetattr(0,&saved)){free(d);return 1;}raw=saved;raw.c_lflag&=~(ICANON|ECHO);raw.c_iflag&=~(IXON|ICRNL);raw.c_cc[VMIN]=0;raw.c_cc[VTIME]=0;raw.c_cc[VSUSP]=_POSIX_VDISABLE;
 struct sigaction action={0};sigemptyset(&action.sa_mask);action.sa_handler=signal_stop;sigaction(SIGINT,&action,NULL);sigaction(SIGHUP,&action,NULL);sigaction(SIGTERM,&action,NULL);action.sa_handler=signal_resize;sigaction(SIGWINCH,&action,NULL);
 if(tcsetattr(0,TCSANOW,&raw)){free(d);return 1;}
 char status[160]="UTF-8 text; tabs shown as >. No files are changed.",search[81]={0};int match=-1;
 for(;;){draw(d,argv[1],status);int c=input();if(c==-2)continue;if(c<0||c=='q'||c=='Q'||c==17)break;unsigned rows,cols;geometry(&rows,&cols);if(rows<6||cols<20)continue;unsigned view=rows-4;
  if(c=='/'){char newquery[81];int rc=query(newquery);if(rc<0)break;if(!rc)continue;strcpy(search,newquery);match=nova_read_find(d,search,0,view);snprintf(status,sizeof status,match>=0?"Match found. n: next match (ASCII, case-sensitive).":"No match. /: new search; q: close.");}
  else if(c=='n'&&search[0]){int next=nova_read_find(d,search,match>=0?(size_t)match+1:0,view);if(next<0)snprintf(status,sizeof status,"No further match; / searches from the start.");else{match=next;snprintf(status,sizeof status,"Next match. n: continue.");}}
  else if(c==' '||c=='f')nova_read_move(d,(int)view,view);
  else if(c=='b')nova_read_move(d,-(int)view,view);
  else if(c=='j')nova_read_move(d,1,view);else if(c=='k')nova_read_move(d,-1,view);
  else if(c=='g')d->top=0;else if(c=='G')d->top=nova_read_last(d,view);
  else if(c==27){switch(arrow()){case 1:nova_read_move(d,-1,view);break;case 2:nova_read_move(d,1,view);break;case 3:nova_read_move(d,-(int)view,view);break;case 4:nova_read_move(d,(int)view,view);break;case 5:d->top=0;break;case 6:d->top=nova_read_last(d,view);break;}}
 }
 int result=stopped?128+stopped:0;if(tcsetattr(0,TCSANOW,&saved)&&!result)result=1;printf("\033[2J\033[HDocument closed; no file changed.\r\n");fflush(stdout);free(d);return result;
}
