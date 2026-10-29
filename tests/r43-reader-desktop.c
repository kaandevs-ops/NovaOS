#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
static int keyfd[2];
static int compare_ms(const void*a,const void*b){double x=*(const double*)a,y=*(const double*)b;return x<y?-1:x>y;}
static void key(unsigned code,int value){struct nova_input_event e={.type=NOVA_EV_KEY,.code=code,.value=value};assert(write(keyfd[1],&e,sizeof e)==sizeof e);consume_input_fd(keyfd[0],1600,900);}
static void toggle(void){key(NOVA_KEY_LEFTCTRL,1);key(56,1);key(NOVA_KEY_T,1);key(NOVA_KEY_T,0);key(56,0);key(NOVA_KEY_LEFTCTRL,0);}
static void tick(void){nova_term_reap();nova_term_flush();struct nova_pollfd f={g_terminal.fd,NOVA_POLLIN,0};struct nova_timespec t={0,10000000};nova_poll(&f,1,&t);if(f.revents)nova_term_receive();}
static void type(const char *text){fprintf(stderr,"TYPE: %s\n",text);for(;*text;text++){
 unsigned found=0;
 for(unsigned shift=0;shift<2&&!found;shift++)for(unsigned code=1;code<58;code++)if(nova_term_key_ascii(code,shift,0)==(unsigned char)(*text=='\n'?'\r':*text)){
  if(shift)key(NOVA_KEY_LEFTSHIFT,1);key(code,1);key(code,0);if(shift)key(NOVA_KEY_LEFTSHIFT,0);found=1;break;
 }
 assert(found);tick();
 if(*text=='\n'){
  unsigned long long end=nova_render_clock(1)+8000000000ULL;
  for(;;){unsigned *line=nova_term_live_row(&g_terminal.grid,g_terminal.grid.y);if(g_terminal.grid.x>=6&&line[g_terminal.grid.x-6]=='n'&&line[g_terminal.grid.x-5]=='o'&&line[g_terminal.grid.x-4]=='v'&&line[g_terminal.grid.x-3]=='a'&&line[g_terminal.grid.x-2]=='$'&&line[g_terminal.grid.x-1]==' ')break;if(nova_render_clock(1)>=end){fprintf(stderr,"Prompt timeout after type newline; grid follows\n");for(unsigned yy=0;yy<g_terminal.grid.rows;yy++){for(unsigned xx=0;xx<g_terminal.grid.cols;xx++)fputc((char)nova_term_live_row(&g_terminal.grid,yy)[xx],stderr);fputc('\n',stderr);}assert(0);}tick();}
 }
}}
static int row_prefix(const char *s){for(unsigned y=0;y<g_terminal.grid.rows;y++){unsigned x=0;while(s[x]&&x<g_terminal.grid.cols&&nova_term_live_row(&g_terminal.grid,y)[x]==(unsigned char)s[x])x++;if(!s[x])return 1;}return 0;}
static void wait_line(const char *s){unsigned long long end=nova_render_clock(1)+8000000000ULL;while(!row_prefix(s)&&nova_render_clock(1)<end)tick();if(!row_prefix(s)){printf("MISSING LINE: %s state=%u error=%u rows=%u cols=%u\n",s,g_terminal.ready,g_terminal.error,g_terminal.rows,g_terminal.cols);for(unsigned y=0;y<g_terminal.grid.rows;y++){for(unsigned x=0;x<g_terminal.grid.cols;x++)putchar((char)nova_term_live_row(&g_terminal.grid,y)[x]);putchar('\n');}}assert(row_prefix(s));}
static int fd_count(void){DIR*d=opendir("/proc/self/fd");assert(d);int n=0;while(readdir(d))n++;closedir(d);return n;}
static void save(const char *dir,unsigned w,unsigned h,unsigned theme){
 char path[4096];snprintf(path,sizeof path,"%s/terminal-%u-%ux%u.HOST-NOT-VM.ppm",dir,theme,w,h);
 size_t size=(size_t)w*h*4;unsigned char *guard=malloc(size+128);assert(guard);memset(guard,0xa5,size+128);
 g_session_theme.choice=theme;g_r2_canvas_width=w;g_r2_canvas_height=h;r25_terminal_layout(w,h);
 nova_term_flush();for(unsigned i=0;i<8;i++)tick();draw_scene(guard+64,w,h,w*4,0);
 for(unsigned i=0;i<64;i++)assert(guard[i]==0xa5&&guard[size+64+i]==0xa5);
 FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);
 unsigned *pixels=(unsigned*)(guard+64);for(unsigned i=0;i<w*h;i++){unsigned char rgb[]={pixels[i]>>16,pixels[i]>>8,pixels[i]};assert(fwrite(rgb,1,3,f)==3);}assert(fclose(f)==0);free(guard);
}

static void enter(void){key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);}
static void control(unsigned code){key(NOVA_KEY_LEFTCTRL,1);key(code,1);key(code,0);key(NOVA_KEY_LEFTCTRL,0);}
int main(int argc,char**argv){
 setvbuf(stdout,NULL,_IONBF,0);assert(argc==2&&getuid()==0);assert(pipe2(keyfd,O_NONBLOCK|O_CLOEXEC)==0);int before=fd_count();
 g_scene.configured=1;g_scene.panel_height=38;g_r5_scale=1;g_r5_logical_w=1600;g_r5_logical_h=900;g_r2_canvas_width=1600;g_r2_canvas_height=900;g_desktop.connected=1;
 toggle();r25_terminal_layout(1600,900);unsigned long long end=nova_render_clock(1)+10000000000ULL;while(!g_terminal.ready&&nova_render_clock(1)<end)tick();assert(g_terminal.ready);wait_line("nova$ ");
 type("i=1; while [ \"$i\" -le 80 ]; do printf 'Line-%03d: real file and kernel permissions\\n' \"$i\"; i=$((i+1)); done > lecture.txt\n");
 type("nova-read lecture.txt");enter();wait_line("Nova Read");wait_line("Line-001: real file");
 int fd=g_terminal.fd;long pid=g_terminal.child;int x=g_terminal.x,y=g_terminal.y,w=g_terminal.width,h=g_terminal.height;
 assert(r25_terminal_click(x+w-86,y+20));assert(g_terminal.maximized&&g_terminal.width==1568&&g_terminal.height==760);assert(g_terminal.fd==fd&&g_terminal.child==pid);
 for(unsigned i=0;i<30;i++)tick();wait_line("Nova Read");
 unsigned *frame=calloc(1600U*900U,4);assert(frame);double timing[121];for(unsigned i=0;i<20;i++)draw_scene(frame,1600,900,6400,i);for(unsigned i=0;i<121;i++){unsigned long long t=nova_render_clock(1);draw_scene(frame,1600,900,6400,i);timing[i]=(double)(nova_render_clock(1)-t)/1000000.0;}qsort(timing,121,sizeof(double),compare_ms);free(frame);struct rusage usage;assert(!getrusage(RUSAGE_SELF,&usage));char perfpath[4096];snprintf(perfpath,sizeof perfpath,"%s/reader-performance.json",argv[1]);FILE*pf=fopen(perfpath,"w");assert(pf);fprintf(pf,"{\"scope\":\"host native maximized reader;not FPS/guest/not paired R42 comparison\",\"p50_ms\":%.6f,\"p95_ms\":%.6f,\"harness_peak_rss_kib\":%ld}\n",timing[60],timing[114],usage.ru_maxrss);assert(!fclose(pf));
 const unsigned dims[][2]={{1600,900},{1280,800},{1920,1080},{400,400}};
 for(unsigned i=0;i<4;i++)for(unsigned t=0;t<3;t++)save(argv[1],dims[i][0],dims[i][1],t);
 g_r2_canvas_width=1600;g_r2_canvas_height=900;r25_terminal_layout(1600,900);for(unsigned i=0;i<30;i++)tick();
 assert(r25_terminal_click(g_terminal.x+g_terminal.width-86,g_terminal.y+20));assert(!g_terminal.maximized&&g_terminal.x==x&&g_terminal.y==y&&g_terminal.width==w&&g_terminal.height==h);assert(g_terminal.fd==fd&&g_terminal.child==pid);
 type(" ");for(unsigned i=0;i<20;i++)tick();assert(!row_prefix("Line-001: real file"));key(102,1);key(102,0);wait_line("Line-001: real file");
 g_terminal.focused=0;toggle();assert(g_terminal.visible&&g_terminal.focused&&g_terminal.fd==fd&&g_terminal.child==pid);
 type("q");wait_line("Document closed;");wait_line("nova$ ");type("test -s lecture.txt && echo REAL_SOURCE_UNCHANGED\n");wait_line("REAL_SOURCE_UNCHANGED");
 assert(r25_terminal_click(g_terminal.x+g_terminal.width-20,g_terminal.y+20));end=nova_render_clock(1)+5000000000ULL;while(g_terminal.child>0&&nova_render_clock(1)<end)tick();assert(g_terminal.child==0&&fd_count()==before);
 close(keyfd[0]);close(keyfd[1]);puts("R43_NATIVE_READER_PASS real shell/PTY/read-only document, maximize/restore/screen-boundaries/resize/focus/FD;12 guarded frames HOST NOT VM");return 0;
}
