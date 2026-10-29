#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "../terminal/nova_home_file.h"
#include "../terminal/nova_user_path.h"
#include "nova_pdf_protocol.h"
#include "nova_pdf_sixel.h"
#ifndef NOVA_PDF_WORKER
#define NOVA_PDF_WORKER "/bin/nova-pdf-worker"
#endif
static volatile sig_atomic_t stopped,resized;
static void stop(int s){if(s==SIGWINCH)resized=1;else stopped=1;}
static long long millis(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return (long long)t.tv_sec*1000+t.tv_nsec/1000000;}
static int render(int snapshot,unsigned page,unsigned w,unsigned h,unsigned zoom,unsigned px,unsigned py,struct nova_pdf_result*out,unsigned char*rgb){
 int pipes[2];if(pipe2(pipes,O_CLOEXEC))return -1;pid_t pid=fork();if(pid<0){close(pipes[0]);close(pipes[1]);return -1;}
 if(!pid){char a[6][24];unsigned v[]={page,w,h,zoom,px,py};for(int i=0;i<6;i++)snprintf(a[i],sizeof a[i],"%u",v[i]);if(dup2(pipes[1],1)<0||dup2(snapshot,3)<0)_exit(126);if(fcntl(3,F_SETFD,0)<0)_exit(126);for(int i=4;i<64;i++)close(i);close(0);close(2);execl(NOVA_PDF_WORKER,NOVA_PDF_WORKER,a[0],a[1],a[2],a[3],a[4],a[5],(char*)NULL);_exit(127);}
 close(pipes[1]);size_t got=0,need=sizeof(*out);int valid=0,eof=0,status=0;long long deadline=millis()+5000;
 while(!stopped&&!resized&&millis()<deadline){struct pollfd f={pipes[0],POLLIN|POLLHUP,0};int n=poll(&f,1,50);if(n<0&&errno==EINTR)continue;if(n<0)break;if(!n)continue;
  unsigned char extra;void*dst=got<sizeof(*out)?(unsigned char*)out+got:got<need?rgb+got-sizeof(*out):&extra;size_t count=got<sizeof(*out)?sizeof(*out)-got:got<need?need-got:1;ssize_t z=read(pipes[0],dst,count);if(z==0){eof=1;break;}if(z<0){if(errno==EINTR)continue;break;}if(got>=need&&valid)break;got+=(size_t)z;
  if(got==sizeof(*out)&&!valid){if(!nova_pdf_result_valid(out,page,w,h))break;valid=1;need=sizeof(*out)+out->bytes;}
 }
 close(pipes[0]);/* Never wait indefinitely for a child that closed its pipe. */
 int reaped=0;while(!stopped&&!resized&&millis()<deadline){pid_t r=waitpid(pid,&status,WNOHANG);if(r==pid){reaped=1;break;}if(r<0&&errno!=EINTR)break;poll(NULL,0,10);}
 if(!reaped){kill(pid,SIGKILL);while(waitpid(pid,&status,0)<0&&errno==EINTR){}return -1;}
 if(!(valid&&eof&&got==need&&WIFEXITED(status)&&!WEXITSTATUS(status))){fprintf(stderr,"PDF worker failed (exit=%d, signal=%d, bytes=%zu).\n",WIFEXITED(status)?WEXITSTATUS(status):-1,WIFSIGNALED(status)?WTERMSIG(status):0,got);return -1;}return 0;
}
int main(int argc,char**argv){
 if(argc!=2){fprintf(stderr,"Usage: nova-pdf HOME-PATH\n");return 2;}int root=open("/home/nova",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);if(root<0||nova_text_root(root)){perror("PDF home");if(root>=0)close(root);return 1;}
 char name[128];int parent=nova_user_parent(root,"/home/nova",argv[1],name);int path_error=errno;close(root);if(parent<0){fprintf(stderr,"PDF: unsafe or inaccessible home path\n");return path_error==EINVAL||path_error==EPERM||path_error==ENAMETOOLONG?2:1;}
 int file=openat(parent,name,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);close(parent);struct stat st;if(file<0||fstat(file,&st)||!S_ISREG(st.st_mode)||st.st_uid!=geteuid()||st.st_nlink!=1||st.st_size<8||st.st_size>NOVA_PDF_MAX_BYTES){fprintf(stderr,"PDF: inaccessible, unsafe or larger than 4 MiB\n");if(file>=0)close(file);return 1;}
 int snapshot=memfd_create("nova-pdf",0x13);if(snapshot<0){close(file);return 1;}char buf[8192];size_t total=0;int failed=0;while(total<(size_t)st.st_size){size_t n=(size_t)st.st_size-total;if(n>sizeof buf)n=sizeof buf;ssize_t z=read(file,buf,n);if(z<0&&errno==EINTR)continue;if(z<=0){failed=1;break;}if(write(snapshot,buf,(size_t)z)!=z){failed=1;break;}total+=(size_t)z;}close(file);
 if(failed||fchmod(snapshot,0400)||fcntl(snapshot,F_ADD_SEALS,0x2f)<0){close(snapshot);return 1;}
 struct termios old,raw;if(tcgetattr(0,&old)){close(snapshot);fprintf(stderr,"PDF requires a terminal\n");return 1;}raw=old;raw.c_lflag&=~(ICANON|ECHO);raw.c_iflag&=~(IXON|ICRNL);raw.c_cc[VMIN]=0;raw.c_cc[VTIME]=1;if(tcsetattr(0,TCSAFLUSH,&raw)){close(snapshot);return 1;}
 struct sigaction sa={0};sa.sa_handler=stop;sigemptyset(&sa.sa_mask);sigaction(SIGWINCH,&sa,NULL);sigaction(SIGINT,&sa,NULL);sigaction(SIGTERM,&sa,NULL);sigaction(SIGHUP,&sa,NULL);signal(SIGPIPE,SIG_IGN);
 unsigned char*rgb=malloc(800*600*3);struct pdf_packet packet={malloc(1048576),0,1048576};int rc=0;unsigned page=0,zoom=1000,px=0,py=0,pages=0;int dirty=1;struct nova_pdf_result out={0};
 if(!rgb||!packet.data){rc=1;goto done;}
 while(!stopped){if(resized){resized=0;dirty=1;}if(dirty){struct winsize ws={0};ioctl(0,TIOCGWINSZ,&ws);if((ws.ws_row&&ws.ws_row<10)||(ws.ws_col&&ws.ws_col<28)){printf("\033[2J\033[HEnlarge window for PDF.\r\nq: exit\r\n");fflush(stdout);dirty=0;goto input;}unsigned w=ws.ws_col?ws.ws_col*9:720,h=ws.ws_row>5?(ws.ws_row-5)*18:180;if(w>800)w=800;if(h>600)h=600;if(w<90)w=90;if(h<72)h=72;
   if(render(snapshot,page,w,h,zoom,px,py,&out,rgb)){if(resized)continue;fprintf(stdout,"\033[2J\033[HPDF could not be rendered within the safety limits.\r\n");rc=1;break;}pages=out.pages;px=out.pan_x;py=out.pan_y;
   if(pdf_sixel(&packet,rgb,out.width,out.height)){fprintf(stdout,"\033[2J\033[HPDF image exceeds the terminal packet budget.\r\n");rc=1;break;}
   printf("\033[2J\033[HNova PDF  |  Page %u/%u  |  Zoom %u%%\r\nn/Space next  b previous  +/- zoom  arrows pan  q exit\r\n",page+1,pages,zoom/10);fflush(stdout);if(fwrite(packet.data,1,packet.size,stdout)!=packet.size){rc=1;break;}printf("\033[%u;1H",ws.ws_row?ws.ws_row:24);fflush(stdout);dirty=0;
  }
  input:;unsigned char key;ssize_t n=read(0,&key,1);if(n<0&&errno==EINTR)continue;if(n<0){rc=1;break;}if(!n){struct pollfd f={0,POLLIN,0};poll(&f,1,50);if(f.revents&POLLHUP)break;continue;}
  if(key=='q'||key==17)break;if((key=='n'||key==' ')&&page+1<pages){page++;px=py=0;dirty=1;}if(key=='b'&&page){page--;px=py=0;dirty=1;}if(key=='+'&&zoom<4000){zoom+=500;dirty=1;}if(key=='-'&&zoom>1000){zoom-=500;dirty=1;}
  if(key==27){unsigned char seq[2];struct pollfd f={0,POLLIN,0};if(poll(&f,1,100)>0&&read(0,seq,1)==1&&seq[0]=='['&&poll(&f,1,100)>0&&read(0,seq+1,1)==1){if(seq[1]=='C')px+=64;if(seq[1]=='D')px=px>64?px-64:0;if(seq[1]=='B')py+=64;if(seq[1]=='A')py=py>64?py-64:0;if(px>3200)px=3200;if(py>2400)py=2400;dirty=1;}}
 }
 done:tcsetattr(0,TCSAFLUSH,&old);if(!rc)printf("\033[2J\033[H");fflush(stdout);free(rgb);free(packet.data);close(snapshot);return rc;
}
