#define _GNU_SOURCE
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/socket.h>
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
 for(unsigned mode=0;mode<4&&!found;mode++)for(unsigned code=1;code<87;code++)if(nova_trq_key(code,mode&1,0,mode>>1,0)==(unsigned char)(*text=='\n'?'\r':*text)){
  if(mode&1)key(NOVA_KEY_LEFTSHIFT,1);if(mode>>1)key(100,1);key(code,1);key(code,0);if(mode>>1)key(100,0);if(mode&1)key(NOVA_KEY_LEFTSHIFT,0);found=1;break;
 }
 assert(found);tick();
 if(*text=='\n'){
  unsigned long long end=nova_render_clock(1)+8000000000ULL;
  for(;;){unsigned *line=nova_term_live_row(&g_terminal.grid,g_terminal.grid.y);if(g_terminal.grid.x==6&&line[0]=='n'&&line[1]=='o'&&line[2]=='v'&&line[3]=='a'&&line[4]=='$'&&line[5]==' ')break;if(nova_render_clock(1)>=end){fprintf(stderr,"PROMPT TIMEOUT ready=%u fd=%d modal=%u focus=%u desktop_focus=%u\n",g_terminal.ready,g_terminal.fd,g_desktop_document_visible,g_terminal.focused,g_desktop_has_focus);for(unsigned row=0;row<g_terminal.grid.rows;row++){for(unsigned col=0;col<g_terminal.grid.cols;col++)fputc((char)nova_term_live_row(&g_terminal.grid,row)[col],stderr);fputc(10,stderr);}abort();}tick();}
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
static void file_wait(void){unsigned long long end=nova_render_clock(1)+8000000000ULL;while((g_desktop_file_pending||g_desktop_file_inflight||g_desktop_pending||g_desktop_inflight)&&nova_render_clock(1)<end)tick();assert(!r54_desktop_busy()&&!g_desktop_inflight&&!g_desktop_pending);}
static unsigned entry(const char*name){for(unsigned i=0;i<g_desktop_files.count;i++)if(nova_streq(g_desktop_files.entries[i].name,name))return i;return ~0U;}
static void desktop_click(const char*name){unsigned i=entry(name);assert(i!=~0U);unsigned rows=(g_r2_canvas_height-170)/100;g_render.cursor_x=28+(int)(i/rows)*112+50;g_render.cursor_y=58+(int)(i%rows)*100+35;key(NOVA_BTN_LEFT,1);key(NOVA_BTN_LEFT,0);}
int main(int argc,char**argv){
 setvbuf(stdout,NULL,_IONBF,0);assert(argc==2&&getuid()==0);assert(!pipe2(keyfd,O_NONBLOCK|O_CLOEXEC));
 g_scene.configured=1;g_scene.panel_height=38;g_r5_scale=1;g_r5_logical_w=1600;g_r5_logical_h=900;g_r2_canvas_width=1600;g_r2_canvas_height=900;g_desktop.connected=1;
 r48_desktop_request(1);for(unsigned i=0;i<1000&&(!g_terminal.ready||g_desktop_pending||g_desktop_inflight);i++)tick();assert(g_terminal.ready&&!g_terminal.visible);wait_line("nova$ ");int child=g_terminal.child;
 toggle();r25_terminal_layout(1600,900);
 type("printf 'NOVAOS_REAL_TEXT\\nT\\303\\274rk\\303\\247e: \\304\\261 \\304\\260 \\303\\247 \\305\\237 \\304\\237 \\303\\266 \\303\\274\\n' > Desktop/Deneme.txt\nyes LONG_REAL_TEXT | head -n 160 > Desktop/Long.txt\n");
 r48_desktop_request(1);file_wait();assert(entry("Deneme.txt")!=~0U&&entry("Long.txt")!=~0U);toggle();assert(!g_terminal.visible);
 desktop_click("Deneme.txt");key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);file_wait();assert(g_desktop_document_visible&&!g_desktop_preview.status&&g_desktop_preview.length>0&&strstr((char*)g_desktop_preview.text,"NOVAOS_REAL_TEXT"));assert(g_terminal.child==child);
 /* User-reported regression: desktop modal must not consume Ctrl+Alt+T. */
 toggle();assert(!g_desktop_document_visible&&!g_desktop_trash_visible&&!g_desktop_menu&&g_terminal.visible&&g_terminal.focused&&g_terminal.child==child);
 type("pwd\n");wait_line("/home/nova");toggle();g_desktop_last_click_name[0]=0;desktop_click("Deneme.txt");key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);file_wait();assert(g_desktop_document_visible);
 const unsigned dims[][2]={{1280,800},{1600,900},{1920,1080},{640,480}};
 for(unsigned d=0;d<4;d++)for(unsigned theme=0;theme<3;theme++)save(argv[1],dims[d][0],dims[d][1],theme);
 key(NOVA_KEY_ESC,1);key(NOVA_KEY_ESC,0);assert(!g_desktop_document_visible);g_r2_canvas_width=1600;g_r2_canvas_height=900;
 desktop_click("Long.txt");key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);file_wait();assert(g_desktop_preview.total>NOVA_DESKTOP_TEXT_BYTES);key(109,1);key(109,0);file_wait();assert(g_desktop_preview.offset>0);key(104,1);key(104,0);file_wait();assert(!g_desktop_preview.offset);key(NOVA_KEY_ESC,1);key(NOVA_KEY_ESC,0);
 /* A real Delete key moves only the selected Desktop basename. */
 g_desktop_last_click_name[0]=0;desktop_click("Deneme.txt");key(111,1);key(111,0);file_wait();assert(entry("Deneme.txt")==~0U&&!g_desktop_file_error);
 g_render.cursor_x=1600-74;g_render.cursor_y=900-160;key(NOVA_BTN_LEFT,1);key(NOVA_BTN_LEFT,0);file_wait();assert(g_desktop_trash_visible&&g_desktop_trash_files.count==1&&nova_streq(g_desktop_trash_files.entries[0].name,"Deneme.txt"));
 toggle();assert(!g_desktop_trash_visible&&g_terminal.visible&&g_terminal.focused&&g_terminal.child==child);toggle();g_render.cursor_x=1600-74;g_render.cursor_y=900-160;key(NOVA_BTN_LEFT,1);key(NOVA_BTN_LEFT,0);file_wait();assert(g_desktop_trash_visible);
 key(NOVA_KEY_DOWN,1);key(NOVA_KEY_DOWN,0);
 for(unsigned d=0;d<4;d++)for(unsigned theme=0;theme<3;theme++){
  char sub[4096];snprintf(sub,sizeof sub,"%s/trash",argv[1]);assert(mkdir(sub,0755)==0||errno==EEXIST);save(sub,dims[d][0],dims[d][1],theme);
 }
 g_r2_canvas_width=1600;g_r2_canvas_height=900;key(NOVA_KEY_DOWN,1);key(NOVA_KEY_DOWN,0);key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);file_wait();assert(!g_desktop_trash_files.count&&entry("Deneme.txt")!=~0U&&!g_desktop_file_error);
 key(NOVA_KEY_ESC,1);key(NOVA_KEY_ESC,0);g_desktop_last_click_name[0]=0;
 /* Actual desktop drag/release to the visible trash control. */
 unsigned note=entry("Deneme.txt"),rows=(900-170)/100;assert(note!=~0U);g_render.cursor_x=28+(int)(note/rows)*112+50;g_render.cursor_y=58+(int)(note%rows)*100+35;key(NOVA_BTN_LEFT,1);g_render.cursor_x=1600-74;g_render.cursor_y=900-160;key(NOVA_BTN_LEFT,0);file_wait();assert(entry("Deneme.txt")==~0U&&!g_desktop_file_error);
 toggle();type("echo COLLISION_NEW > Desktop/Deneme.txt\n");r48_desktop_request(1);file_wait();toggle();g_render.cursor_x=1600-74;g_render.cursor_y=900-160;key(NOVA_BTN_LEFT,1);key(NOVA_BTN_LEFT,0);file_wait();assert(g_desktop_trash_files.count==1);key(NOVA_KEY_DOWN,1);key(NOVA_KEY_DOWN,0);key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);file_wait();assert(g_desktop_file_error==17&&g_desktop_trash_files.count==1);
 for(unsigned d=0;d<4;d++)for(unsigned theme=0;theme<3;theme++){
  char sub[4096];snprintf(sub,sizeof sub,"%s/collision",argv[1]);assert(mkdir(sub,0755)==0||errno==EEXIST);save(sub,dims[d][0],dims[d][1],theme);
 }
 g_r2_canvas_width=1600;g_r2_canvas_height=900;

 key(NOVA_KEY_ESC,1);key(NOVA_KEY_ESC,0);toggle();type("cat Desktop/Deneme.txt\nmv Desktop/Deneme.txt Desktop/Yeni-Kopya.txt\n");wait_line("COLLISION_NEW");r48_desktop_request(1);file_wait();toggle();g_render.cursor_x=1600-74;g_render.cursor_y=900-160;key(NOVA_BTN_LEFT,1);key(NOVA_BTN_LEFT,0);file_wait();key(NOVA_KEY_DOWN,1);key(NOVA_KEY_DOWN,0);key(NOVA_KEY_ENTER,1);key(NOVA_KEY_ENTER,0);file_wait();assert(!g_desktop_file_error&&!g_desktop_trash_files.count&&entry("Deneme.txt")!=~0U);
 key(NOVA_KEY_ESC,1);key(NOVA_KEY_ESC,0);g_desktop_last_click_name[0]=0;desktop_click("Deneme.txt");
 /* True double click, without Enter, opens the same actual file. */
 desktop_click("Deneme.txt");file_wait();assert(g_desktop_document_visible&&g_desktop_preview.length>0);key(NOVA_KEY_ESC,1);key(NOVA_KEY_ESC,0);
 toggle();type("cat Desktop/Deneme.txt\n");wait_line("NOVAOS_REAL_TEXT");
 type("nova-pkg list\nnova-pkg install lua\nnova-pkg run lua -e 'print(\"R54_APPLICATION_OK\")'\n");wait_line("R54_APPLICATION_OK");
 type("nova-pkg status lua\n");wait_line("lua: installed, verified");
 type("nova-pkg remove lua\ntest ! -e .nova-apps/lua && echo R54_REMOVED_OK\n");wait_line("R54_REMOVED_OK");puts("PASS real fixed-catalog Lua install/run/status/remove in the same isolated user session");
 nova_term_disconnect(0);unsigned long long end=nova_render_clock(1)+4000000000ULL;while(g_terminal.child&&nova_render_clock(1)<end){nova_term_reap();usleep(10000);}assert(!g_terminal.child);
 int bad[2];assert(!socketpair(AF_UNIX,SOCK_SEQPACKET,0,bad));g_terminal.fd=bad[0];struct nova_term_packet packet={.magic=NOVA_TERM_MAGIC,.version=1,.op=NOVA_TERM_DESKTOP_FILE_STATE,.length=sizeof(struct nova_desktop_file_reply)};struct nova_desktop_file_reply forged={.action=NOVA_DESKTOP_PREVIEW,.length=NOVA_DESKTOP_TEXT_BYTES+1,.total=2000};memcpy(packet.data,&forged,sizeof forged);g_desktop_file_inflight=NOVA_DESKTOP_PREVIEW;assert(send(bad[1],&packet,NOVA_TERM_HEADER+packet.length,0)>0);assert(nova_term_receive()&&g_terminal.fd==-1&&g_terminal.error==71);close(bad[1]);
 close(keyfd[0]);close(keyfd[1]);puts("PASS production input -> isolated PTY worker -> real preview/page/trash/restore; 36 guarded HOST native frames; forged reply rejected; same shell survived");return 0;
}
