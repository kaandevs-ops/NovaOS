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
static void type(const char *text){for(;*text;text++){
 unsigned found=0;
 for(unsigned shift=0;shift<2&&!found;shift++)for(unsigned code=1;code<58;code++)if(nova_term_key_ascii(code,shift,0)==(unsigned char)(*text=='\n'?'\r':*text)){
  if(shift)key(NOVA_KEY_LEFTSHIFT,1);key(code,1);key(code,0);if(shift)key(NOVA_KEY_LEFTSHIFT,0);found=1;break;
 }
 assert(found);tick();
 if(*text=='\n'){
  unsigned long long end=nova_render_clock(1)+8000000000ULL;
  for(;;){unsigned *line=nova_term_live_row(&g_terminal.grid,g_terminal.grid.y);if(g_terminal.grid.x==6&&line[0]=='n'&&line[1]=='o'&&line[2]=='v'&&line[3]=='a'&&line[4]=='$'&&line[5]==' ')break;assert(nova_render_clock(1)<end);tick();}
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
int main(int argc,char **argv){
 setvbuf(stdout,NULL,_IONBF,0);assert(argc==2&&getuid()==0);assert(pipe2(keyfd,O_NONBLOCK|O_CLOEXEC)==0);int fds=fd_count();
 g_scene.configured=1;g_scene.panel_height=38;g_r5_scale=1;g_r5_logical_w=1600;g_r5_logical_h=900;g_r2_canvas_width=1600;g_r2_canvas_height=900;g_desktop.connected=1;
 toggle();assert(g_terminal.visible&&g_terminal.fd>=0);r25_terminal_layout(1600,900);
 unsigned long long end=nova_render_clock(1)+8000000000ULL;while(!g_terminal.ready&&nova_render_clock(1)<end)tick();assert(g_terminal.ready);
 wait_line("nova$ ");type("id\npwd\nuname -s\necho native-proof > note.txt\ncat note.txt\nprintf 'b\\na\\n' | sort\n");wait_line("native-proof");wait_line("uid=12004");wait_line("/home/nova");wait_line("Linux");
 type("echo history-proof\n");key(NOVA_KEY_UP,1);key(NOVA_KEY_UP,0);type("\n");
 unsigned histories=0;for(unsigned y=0;y<g_terminal.grid.rows;y++){unsigned *row=nova_term_live_row(&g_terminal.grid,y);const char *expected="history-proof";unsigned i=0;while(expected[i]&&row[i]==(unsigned char)expected[i])i++;if(!expected[i])histories++;}assert(histories>=2);
 type("echo editX");key(NOVA_KEY_LEFT,1);key(NOVA_KEY_LEFT,0);key(NOVA_KEY_DELETE,1);key(NOVA_KEY_DELETE,0);type("-proof\n");wait_line("edit-proof");
 assert(g_terminal.cols==103&&g_terminal.rows==23);nova_term_flush();
 toggle();assert(!g_terminal.visible&&g_terminal.fd>=0);toggle();assert(g_terminal.visible&&g_terminal.ready);
 type("cat note.txt\n");wait_line("native-proof");
 /* Regression: focus loss is not minimization, Alt+Tab preserves the PTY. */
 int session_fd=g_terminal.fd;long session_pid=g_terminal.child;
 assert(!r25_terminal_click(0,700));assert(g_terminal.visible&&!g_terminal.focused);
 assert(r25_terminal_click(g_terminal.x+30,g_terminal.y+25));assert(g_terminal.focused);
 g_surfaces[0].used=1;g_surfaces[0].committed=1;g_surfaces[0].surface_id=900;
 g_surfaces[0].x=20;g_surfaces[0].y=100;g_surfaces[0].width=200;g_surfaces[0].height=200;
 key(56,1);key(NOVA_KEY_TAB,1);key(NOVA_KEY_TAB,0);key(56,0);
 assert(g_terminal.visible&&!g_terminal.focused&&g_client.focus_surface==900);
 /* Shortcut from another app activates in one step and preserves session. */
 toggle();assert(g_terminal.visible&&g_terminal.focused&&!g_client.focus_surface);
 assert(g_terminal.fd==session_fd&&g_terminal.child==session_pid);
 toggle();assert(!g_terminal.visible&&g_terminal.fd==session_fd);
 toggle();assert(g_terminal.visible&&g_terminal.focused);
 set_client_focus(900);
 assert(g_terminal.visible&&!g_terminal.focused&&g_client.focus_surface==900);

 key(56,1);key(NOVA_KEY_TAB,1);key(NOVA_KEY_TAB,0);key(56,0);
 assert(g_terminal.visible&&g_terminal.focused&&!g_client.focus_surface);
 assert(g_terminal.fd==session_fd&&g_terminal.child==session_pid);
 assert(nova_term_key_ascii(NOVA_KEY_ESC,0,0)==27);assert(g_terminal.visible);
 nova_zero(&g_surfaces[0],sizeof g_surfaces[0]);
 type("cat note.txt\n");wait_line("native-proof");
 /* Real vi on the isolated PTY: Escape reaches the program, not WM hide. */
 type("vi edited.txt");key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);
 for(unsigned i=0;i<50;i++)tick();
 type("iCreated with real vi");key(NOVA_KEY_ESC,1);key(NOVA_KEY_ESC,0);
 for(unsigned i=0;i<20;i++)tick();type(":wq");key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);
 for(unsigned i=0;i<50;i++)tick();assert(g_terminal.visible);
 type("cat edited.txt\n");wait_line("Created with real vi");
 unsigned *frame=calloc(1600U*900U,4);assert(frame);double times[121];for(unsigned i=0;i<20;i++)draw_scene(frame,1600,900,6400,i);
 for(unsigned i=0;i<121;i++){unsigned long long before=nova_render_clock(1);draw_scene(frame,1600,900,6400,i);times[i]=(double)(nova_render_clock(1)-before)/1000000.0;}qsort(times,121,sizeof(double),compare_ms);free(frame);
 struct rusage usage;assert(!getrusage(RUSAGE_SELF,&usage));char perf_path[4096];snprintf(perf_path,sizeof perf_path,"%s/terminal-render.json",argv[1]);FILE *perf=fopen(perf_path,"w");assert(perf);fprintf(perf,"{\"scope\":\"WSL native terminal with real shell output, 1600x900, 20 warmup, 121 samples; NOT DRM/FPS\",\"p50_ms\":%.6f,\"p95_ms\":%.6f,\"peak_rss_kib\":%ld,\"terminal_state_bytes\":%lu,\"mono_atlas_bytes\":%lu}\n",times[60],times[114],usage.ru_maxrss,(unsigned long)sizeof(g_terminal),(unsigned long)sizeof(nova_term_font_glyphs));assert(!fclose(perf));
 const unsigned dims[][2]={{1280,800},{1600,900},{1920,1080},{400,400}};
 for(unsigned d=0;d<4;d++)for(unsigned theme=0;theme<3;theme++)save(argv[1],dims[d][0],dims[d][1],theme);
 g_r2_canvas_width=1600;g_r2_canvas_height=900;r25_terminal_layout(1600,900);
 assert(r25_terminal_click(g_terminal.x+g_terminal.width-12,g_terminal.y+g_terminal.height-12));assert(g_terminal.grab==2);
 assert(r25_terminal_motion(g_terminal.grab_x+50,g_terminal.grab_y+30,1600,900));g_terminal.grab=0;
 unsigned rows=g_terminal.rows,cols=g_terminal.cols;nova_term_flush();char command[64];snprintf(command,sizeof command,"stty size\n");type(command);char expected[64];snprintf(expected,sizeof expected,"%u %u",rows,cols);wait_line(expected);
 assert(r25_terminal_click(g_terminal.x+g_terminal.width-20,g_terminal.y+20));assert(!g_terminal.visible&&g_terminal.fd<0);
 end=nova_render_clock(1)+4000000000ULL;while(g_terminal.child>0&&nova_render_clock(1)<end){nova_term_reap();struct timespec t={0,10000000};nanosleep(&t,0);}assert(g_terminal.child==0);
 assert(fd_count()==fds);assert(NOVA_MAX_CLIENT_CONNECTIONS==3);close(keyfd[0]);close(keyfd[1]);
 puts("PASS: production compositor key dispatch -> private helper -> real shell -> bounded grid -> native rendering; hide/restore, real resize, close/reap, FD stability; 12 guarded frames (HOST ONLY)");
}
