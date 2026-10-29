#define _GNU_SOURCE
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "nova_text_store.h"
#ifndef NOVA_EDIT_HOME
#define NOVA_EDIT_HOME "/home/nova"
#endif
static volatile sig_atomic_t interrupted,resized;
static void resize_signal(int sig){(void)sig;resized=1;}
static void stop(int sig){interrupted=sig;}
static void size(unsigned*rows,unsigned*cols){struct winsize w={0};ioctl(1,TIOCGWINSZ,&w);*rows=w.ws_row>=4?w.ws_row:24;*cols=w.ws_col>=20?w.ws_col:80;if(*rows>48)*rows=48;if(*cols>120)*cols=120;}
static void position(const struct nova_text_document*d,size_t end,unsigned cols,unsigned*y,unsigned*x){*x=*y=0;for(size_t i=0;i<end;){unsigned k=nova_text_unit(d->data+i,d->length-i);if(d->data[i]=='\n'){(*y)++;*x=0;}else if(++*x>=cols){(*y)++;*x=0;}i+=k;}}
static void draw(const struct nova_text_document*d,const char*dest,const char*status){
 unsigned rows,cols,cy,cx;size(&rows,&cols);if(rows<6){printf("\033[2J\033[H%.*s\r\n%.*s",(int)cols-1,"Nova Edit",(int)cols-1,"Enlarge window; Ctrl+Q exits");fflush(stdout);return;}position(d,d->cursor,cols-1,&cy,&cx);unsigned start=cy>=rows-5?cy-(rows-6):0;
 char title[180];snprintf(title,sizeof title,"Nova Edit | NEW COPY | %s",dest);printf("\033[2J\033[H%.*s\r\n",(int)cols-1,title);
 printf("%.*s\r\n",(int)cols-1,cols<48?"^Ssave ^Oname ^Qexit":"Ctrl+S save | Ctrl+O name | Ctrl+Q cancel | arrows");
 unsigned y=0,x=0;printf("\033[3;1H");
 for(size_t i=0;i<d->length&&y<start+rows-5;){unsigned k=nova_text_unit(d->data+i,d->length-i);unsigned char c=d->data[i];if(c=='\n'){y++;x=0;if(y>start)printf("\r\n");}else{if(y>=start){if(c=='\t')putchar('>');else fwrite(d->data+i,1,k,stdout);}if(++x>=cols-1){y++;x=0;if(y>start)printf("\r\n");}}i+=k;}
 char counter[96];if(cols<48)snprintf(counter,sizeof counter,"%zu/4096 bytes",d->length);else snprintf(counter,sizeof counter,"%zu / 4096 bytes | UTF-8; tab = >",d->length);printf("\033[%u;1H%.*s\033[K",rows-1,(int)cols-1,counter);
 printf("\033[%u;1H%.*s\033[K\033[%u;%uH",rows,(int)cols-1,status,3+cy-start,1+cx);fflush(stdout);
}
static int byte(void){unsigned char c;struct pollfd p={0,POLLIN,0};while(!interrupted){if(resized){resized=0;return -2;}int n=poll(&p,1,150);if(n<0&&errno==EINTR)continue;if(n<0)return -1;if(!n)continue;if(p.revents&(POLLHUP|POLLERR|POLLNVAL))return -1;if(read(0,&c,1)==1)return c;}return -1;}
static int escape(void){unsigned char s[5];unsigned n=0;struct pollfd p={0,POLLIN,0};while(n<sizeof s&&poll(&p,1,60)>0&&read(0,s+n,1)==1){if(s[n]>=0x40&&s[n]<=0x7e&&n)n++;else{n++;continue;}break;}
 if(n==2&&s[0]=='[')return s[1]=='A'?6:s[1]=='B'?7:s[1]=='D'?1:s[1]=='C'?2:s[1]=='H'?3:s[1]=='F'?4:0;
 if(n==3&&s[0]=='['&&s[2]=='~')return s[1]=='1'||s[1]=='7'?3:s[1]=='4'||s[1]=='8'?4:s[1]=='3'?5:0;return 0;
}
/* Save-as changes only the pending flat name, never the text buffer. */
static int choose_name(char out[128]){
 size_t n=0;int invalid=0;out[0]=0;
 for(;;){unsigned rows,cols;size(&rows,&cols);char line[180];snprintf(line,sizeof line,"%sSave as: %s",invalid?"Invalid name. ":"",out);printf("\033[%u;1H%.*s\033[K",rows,(int)cols-1,line);fflush(stdout);int c=byte();if(c==-2)continue;if(c<0)return -1;if(c==27||c==17)return 0;
  if(c==13||c==10){if(nova_text_name(out))return 1;invalid=1;continue;}
  if(c==8||c==127){if(n)out[--n]=0;invalid=0;}else if(c>=32&&c<127&&n<127){out[n++]=(char)c;out[n]=0;invalid=0;}
 }
}
int main(int argc,char**argv){
 const char*source=NULL,*dest=NULL;if(argc==2)dest=argv[1];else if(argc==4&&!strcmp(argv[1],"--copy")){source=argv[2];dest=argv[3];}
 if(!dest||!nova_text_name(dest)||(source&&!nova_text_name(source))){fprintf(stderr,"Usage: nova-edit NEW-NAME | nova-edit --copy SOURCE-NAME NEW-NAME\nHome-only flat names; new file only, no overwrite; up to4096 UTF-8 bytes.\n");return 2;}
 char save_name[128];strcpy(save_name,dest);dest=save_name;
 int root=open(NOVA_EDIT_HOME,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(root<0||nova_text_root(root)){perror("trusted user home");if(root>=0)close(root);return 1;}
 struct nova_text_document d={0};if(source&&nova_text_read(root,source,&d)){perror("read source");close(root);return 1;}
 struct stat st;if(!fstatat(root,dest,&st,AT_SYMLINK_NOFOLLOW)||errno!=ENOENT){fprintf(stderr,"Destination exists or cannot be checked; choose a new name. Nothing overwritten.\n");close(root);return 1;}
 if(!isatty(0)||!isatty(1)){fprintf(stderr,"nova-edit needs a real interactive terminal.\n");close(root);return 2;}
 struct termios saved,raw;if(tcgetattr(0,&saved)){close(root);return 1;}raw=saved;raw.c_lflag&=~(ICANON|ECHO);raw.c_iflag&=~(IXON|ICRNL);raw.c_cc[VMIN]=0;raw.c_cc[VTIME]=0;raw.c_cc[VSUSP]=_POSIX_VDISABLE;
 struct sigaction action={0};action.sa_handler=stop;sigemptyset(&action.sa_mask);sigaction(SIGINT,&action,NULL);sigaction(SIGHUP,&action,NULL);sigaction(SIGTERM,&action,NULL);action.sa_handler=resize_signal;sigaction(SIGWINCH,&action,NULL);
 if(tcsetattr(0,TCSANOW,&raw)){close(root);return 1;}
 char status[256]="No existing file is overwritten. Ctrl+S saves a new 0600 file.";int result=0,published=0;
 unsigned batch=0;
 for(;;){struct pollfd queued={0,POLLIN,0};if(!batch||batch>=32||poll(&queued,1,0)<=0){draw(&d,dest,status);batch=0;}int c=byte();batch++;if(c==-2){batch=32;continue;}if(c<0||c==17){snprintf(status,sizeof status,"Cancelled; no file published.");result=interrupted?128+interrupted:0;break;}
  unsigned current_rows,current_cols;size(&current_rows,&current_cols);(void)current_cols;if(current_rows<6&&c!=19)continue;
  if(c==15){char chosen[128];int rc=choose_name(chosen);if(rc>0){strcpy(save_name,chosen);snprintf(status,sizeof status,"Name changed; Ctrl+S publishes a new file.");}batch=32;continue;}
  if(c==19){unsigned rows,cols;size(&rows,&cols);(void)cols;if(rows<6){snprintf(status,sizeof status,"Enlarge window before saving.");continue;}int rc=nova_text_publish(root,dest,&d);if(rc<0){snprintf(status,sizeof status,"Save refused: %s. Ctrl+O chooses a new name; buffer retained.",strerror(errno));continue;}published=1;if(rc){snprintf(status,sizeof status,"File published, directory sync FAILED: durability unconfirmed.");result=3;}else snprintf(status,sizeof status,"Saved new file: %s (%zu bytes, mode 0600).",dest,d.length);break;}
  if(c==27){switch(escape()){case 1:nova_text_left(&d);break;case 2:nova_text_right(&d);break;case 3:nova_text_home(&d);break;case 4:nova_text_end(&d);break;case 5:nova_text_delete(&d);break;case 6:nova_text_vertical(&d,0);break;case 7:nova_text_vertical(&d,1);break;}continue;}
  if(c==127||c==8){nova_text_backspace(&d);continue;}unsigned char b[4]={(unsigned char)(c==13?10:c)};unsigned n=1;
  if(c>=0xc2&&c<=0xf4){n=c<0xe0?2:c<0xf0?3:4;for(unsigned i=1;i<n;i++){int next=byte();if(next<0){n=i;break;}b[i]=(unsigned char)next;}}
  if(nova_text_insert(&d,b,n))snprintf(status,sizeof status,"Input rejected: valid text only, maximum 4096 bytes.");
 }
 int term_error=tcsetattr(0,TCSANOW,&saved);printf("\033[2J\033[H%s\r\n",status);fflush(stdout);close(root);if(term_error&&!result)result=1;(void)published;return result;
}
