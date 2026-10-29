#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static void ppm(const char*path,unsigned char*b,unsigned w,unsigned h){FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(unsigned i=0;i<w*h;i++){unsigned int c=((unsigned int*)b)[i];unsigned char rgb[]={c>>16,c>>8,c};assert(fwrite(rgb,1,3,f)==3);}assert(!fclose(f));}
int main(int argc,char**argv){assert(argc==2);struct nova_date d={0};assert(!nova_calendar_utc(0,&d)&&d.year==1970&&d.month==1&&d.day==1&&d.weekday==3);assert(!nova_calendar_utc(951782400,&d)&&d.year==2000&&d.month==2&&d.day==29);assert(nova_calendar_days(2100,2)==28&&nova_calendar_days(2000,2)==29);assert(nova_calendar_utc(-1,&d)<0&&nova_calendar_utc(253402300800LL,&d)<0);assert(!nova_calendar_utc(253402300799LL,&d)&&d.year==9999&&d.day==31);
 int yy=1970;unsigned mm=1;assert(nova_calendar_step(&yy,&mm,-1)<0&&yy==1970&&mm==1);yy=9999;mm=12;assert(nova_calendar_step(&yy,&mm,1)<0&&yy==9999&&mm==12);
 for(int year=1970;year<=9999;year++)for(unsigned m=1;m<=12;m++)assert(nova_calendar_first(year,m)<7);
 g_terminal.focused=1;g_r2_canvas_width=1280;g_r2_canvas_height=800;g_scene.panel_height=38;r46_open_time(3);assert(g_r7_controls_visible==3&&!g_terminal.focused);int x,y,w,h;r46_time_layout(1280,800,&x,&y,&w,&h);g_r46_year=2028;g_r46_month=2;assert(r7_chrome_click(x+w-120,y+64)&&g_r46_month==1);assert(r7_chrome_click(x+w-25,y+64)&&g_r46_month==2);assert(r7_chrome_click(x+w-25,y+20)&&!g_r7_controls_visible);
 g_r7_library_visible=1;g_r7_controls_visible=0;g_desktop.connected=0;int lx,ly;unsigned lw,lh;r19_library_layout(1280,800,&lx,&ly,&lw,&lh);assert(r7_chrome_click(lx+40,ly+60)&&g_r7_controls_visible==3&&!g_r7_library_visible);
 g_r7_controls_visible=0;g_r7_library_visible=1;g_r19_menu_selected=1;assert(r19_library_key(NOVA_KEY_ENTER)&&g_r7_controls_visible==4&&!g_r7_library_visible);
 g_r7_library_visible=1;g_desktop.connected=1;g_desktop_tasks.count=1;g_desktop_tasks.entries[0].client_id=3;g_desktop_tasks.entries[0].flags=NOVA_DESKTOP_TASK_FLAG_LAUNCHABLE;r19_library_layout(1280,800,&lx,&ly,&lw,&lh);unsigned id=0,action=0;assert(r7_library_hit(lx+40,ly+60+2*56,&id,&action)&&id==3&&action==NOVA_DESKTOP_ACTION_LAUNCH);g_desktop_tasks.count=0;
 unsigned sizes[][2]={{400,440},{1280,800},{1600,900},{1920,1080}};
 for(unsigned i=0;i<4;i++)for(unsigned theme=0;theme<3;theme++)for(unsigned kind=3;kind<=5;kind++){unsigned ww=sizes[i][0],hh=sizes[i][1];size_t n=(size_t)ww*hh*4;unsigned char*b=malloc(n+64);assert(b);memset(b,0,n+64);memset(b,0x5a,32);memset(b+n+32,0x5a,32);g_session_theme.choice=theme;g_r7_library_visible=1;r3_window(b+32,ww,hh,ww*4,0);for(int guard=0;guard<32;guard++)assert(b[guard]==0x5a&&b[n+32+guard]==0x5a);if(i==1&&theme==0&&kind==3){char path[512];snprintf(path,sizeof path,"%s/applications.ppm",argv[1]);ppm(path,b+32,ww,hh);}memset(b+32,0,n);g_r7_library_visible=0;g_r7_controls_visible=kind;r7_controls(b+32,ww,hh,ww*4);for(int k=0;k<32;k++)assert(b[k]==0x5a&&b[n+32+k]==0x5a);if(i==1&&theme==0){char path[512];snprintf(path,sizeof path,"%s/%s.ppm",argv[1],kind==3?"calendar":kind==4?"clock":"power-confirm");ppm(path,b+32,ww,hh);}free(b);}
 puts("PASS UTC epochs/leap bounds/96360 months/real production click routing/36 guarded native raster scenes; guest PENDING");return 0;}
