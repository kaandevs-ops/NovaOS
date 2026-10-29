#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
static void pump(void){nova_sw_receive();nova_sw_reap();r58_tick();usleep(1000);assert(!g_sw.error);}
static void settled(void){for(unsigned i=0;i<20000;i++){pump();if(g_sw.ready&&!g_sw.pending)return;}assert(!"software timeout");}
static int contains(unsigned app,const char*text){struct nova_term_grid*g=&g_sw.grids[app];char line[512];for(unsigned row=0;row<g->rows;row++){const unsigned*p=nova_term_view_row(g,row);for(unsigned col=0;col<g->cols;col++)line[col]=p[col]<128?(char)p[col]:'?';line[g->cols]=0;if(strstr(line,text))return 1;}return 0;}
static void expect(unsigned app,const char*text){for(unsigned i=0;i<20000;i++){pump();if(contains(app,text))return;}fprintf(stderr,"Missing: %s\n",text);assert(0);}
static void click(int x,int y){assert(r58_click(g_sw.x+x,g_sw.y+y));settled();}
static void snapshot(void){unsigned w=1600,h=900;unsigned char*b=calloc((size_t)w*h,4);assert(b);r58_draw(b,w,h,w*4);nova_r2_dock(b,w,h,w*4);FILE*f=fopen("evidence/r58/software-live.ppm","wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(size_t i=0;i<(size_t)w*h*4;i+=4){fputc(b[i+2],f);fputc(b[i+1],f);fputc(b[i],f);}fclose(f);free(b);}
static unsigned fd_count(void){DIR*d=opendir("/proc/self/fd");assert(d);unsigned n=0;while(readdir(d))n++;closedir(d);return n;}
int main(void){
 unsigned baseline=fd_count();
 g_r2_canvas_width=1600;g_r2_canvas_height=900;r58_layout(1600,900);r58_open(0);settled();assert(g_sw.installed[0]==3);puts("LIVE_READY");
 click(40,g_sw.h-60);assert(g_sw.installed[0]==0);puts("GUI_INSTALL_SQLITE_PASS");
 unsigned ids[4];assert(r58_dock_packages(1600,ids)==1&&ids[0]==0);
 int x=r19_dock_slot_x(r7_dock_start(1600),NOVA_DOCK_SYSTEM_ITEMS+r7_dock_task_count(1600));assert(r7_chrome_click(x+20,r7_dock_top(900)+20));settled();assert(g_sw.running&1);expect(0,"SQLite");puts("DOCK_LAUNCH_PASS");
 const char*query="select 9876;\r";assert(nova_sw_input(0,query,strlen(query)));settled();expect(0,"9876");
 /* Route the actual GUI key event to the owned PTY, preserving primary shell focus. */
 unsigned oldtag=g_sw.tag;assert(r58_key(28,0));assert(g_sw.tag>oldtag||g_sw.queued[0]);settled();assert(!g_terminal.focused);puts("GUI_KEY_ROUTE_PASS");
 for(unsigned i=0;i<100;i++)assert(nova_sw_input(0,"select 31415;\r",14));assert(nova_sw_input(0,".print R58_QUEUE_DRAINED\r",strlen(".print R58_QUEUE_DRAINED\r")));settled();expect(0,"R58_QUEUE_DRAINED");assert(!g_sw.queued[0]);puts("BOUNDED_INPUT_QUEUE_DRAIN_PASS");
 click(35,65);assert(g_sw.page==0);snapshot();
 /* A verified installed app is protected against remove while its process is live. */
 click(270,g_sw.h-60);assert(!g_sw.confirm);assert(g_sw.installed[0]==0);
 r58_app_open(0);settled();click(g_sw.w-25,25);for(unsigned i=0;i<20000&&(g_sw.running&1);i++)pump();assert(!(g_sw.running&1));puts("GUI_CLOSE_PASS");
 r58_open(0);settled();click(270,g_sw.h-60);assert(g_sw.confirm);click(170,g_sw.h-60);assert(g_sw.installed[0]==3);assert(!r58_dock_packages(1600,ids));puts("GUI_REMOVE_AND_DOCK_REFRESH_PASS");
 long child=g_sw.child;nova_sw_down(5);for(unsigned i=0;i<20000&&g_sw.child;i++){nova_sw_reap();usleep(1000);}assert(!g_sw.child&&child>0);
 r58_open(0);settled();assert(g_sw.installed[0]==3);puts("GUI_RECONNECT_PASS");nova_sw_down(5);for(unsigned i=0;i<20000&&g_sw.child;i++){nova_sw_reap();usleep(1000);}assert(!g_sw.child);
 for(unsigned c=0;c<20;c++){r58_open(0);settled();assert(g_sw.installed[0]==3);nova_sw_down(5);for(unsigned i=0;i<20000&&g_sw.child;i++){nova_sw_reap();usleep(1000);}assert(!g_sw.child);assert(fd_count()==baseline);}
 puts("TWENTY_RECONNECT_FD_STABILITY_PASS");puts("R58_LIVE_GUI_PASS");return 0;
}
