#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#define nova_main unused_compositor_main
#ifndef NOVA_BENCH_COMPOSITOR
#define NOVA_BENCH_COMPOSITOR "../src/services/compositor.c"
#endif
#include NOVA_BENCH_COMPOSITOR
#undef nova_main
int main(int argc,char**argv){assert(argc==4);unsigned w=atoi(argv[1]),h=atoi(argv[2]),mode=atoi(argv[3]);assert(w>=640&&w<=1920&&h>=480&&h<=1080);g_scene.configured=1;g_scene.panel_height=38;g_r5_scale=1;g_session_theme.choice=2;g_r2_canvas_width=w;g_r2_canvas_height=h;g_render.cursor_x=200;g_render.cursor_y=200;g_desktop_files.count=2;g_desktop_files.entries[0].kind=1;nova_copy(g_desktop_files.entries[0].name,"Deneme.txt",11);g_desktop_files.entries[1].kind=2;nova_copy(g_desktop_files.entries[1].name,"Belgeler",9);
#ifdef NOVA_BENCH_R54
 if(mode==1){g_desktop_document_visible=1;nova_copy(g_desktop_preview.name,"Deneme.txt",11);for(unsigned i=0;i<1500;i++)g_desktop_preview.text[i]=(i%72==71)?'\n':'a';g_desktop_preview.total=g_desktop_preview.length=1500;}
 if(mode==2){g_desktop_trash_visible=1;g_desktop_trash_files.total=g_desktop_trash_files.count=8;for(unsigned i=0;i<8;i++){g_desktop_trash_files.entries[i].kind=1;nova_copy(g_desktop_trash_files.entries[i].name,"Notlar.txt",11);}}
#else
 assert(mode==0);
#endif
 unsigned char*b=malloc((size_t)w*h*4);assert(b);for(unsigned i=0;i<5;i++)draw_scene(b,w,h,w*4,i);
 printf("{\"ns\":[");for(unsigned i=0;i<60;i++){unsigned long long a=nova_render_clock(1);draw_scene(b,w,h,w*4,i);unsigned long long z=nova_render_clock(1);printf("%s%llu",i?",":"",z-a);}struct rusage usage;assert(!getrusage(RUSAGE_SELF,&usage));printf("],\"peak_rss_kib\":%ld}\n",usage.ru_maxrss);free(b);return 0;
}
