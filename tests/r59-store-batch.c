#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
static void pump(void){if(g_store.fd>=0)nova_store_receive();if(g_sw.fd>=0)nova_sw_receive();nova_store_reap();nova_sw_reap();r58_tick();usleep(1000);assert(!g_store.error&&!g_sw.error);}
static void settle(void){for(unsigned i=0;i<30000;i++){pump();if(g_store.ready&&g_sw.ready&&!g_sw.pending)return;}assert(0);}
static void click(int x,int y){assert(r58_click(g_sw.x+x,g_sw.y+y));settle();}
static void shot(const char*name){unsigned w=1600,h=900;unsigned char*b=calloc((size_t)w*h,4);assert(b);r58_draw(b,w,h,w*4);nova_r2_dock(b,w,h,w*4);char path[512];snprintf(path,sizeof path,"evidence/r59/%s.ppm",name);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(size_t i=0;i<(size_t)w*h*4;i+=4){fputc(b[i+2],f);fputc(b[i+1],f);fputc(b[i],f);}fclose(f);free(b);}
int main(void){g_r2_canvas_width=1600;g_r2_canvas_height=900;r58_layout(1600,900);r58_open(0);settle();assert(g_store.available==15);g_sw.tab=3;assert(r59_store_install_available());for(unsigned n=0;n<15000&&!g_store.bytes;n++)pump();assert(g_store.pending&&r59_store_queue);assert(nova_store_request(NS_CANCEL,g_store.app));assert(!r59_store_queue);for(unsigned n=0;n<15000&&g_store.pending;n++)pump();settle();assert(!g_store.pending&&g_store.result==125);puts("BATCH_CANCEL_STOPS_REMAINING_DOWNLOADS_PASS");assert(r59_store_install_available());for(unsigned n=0;n<60000;n++){pump();if(!g_store.pending&&!r59_store_queue&&!g_sw.pending&&!g_sw.needs_list)break;}assert(!g_store.pending&&!r59_store_queue&&!g_store.result);for(unsigned i=0;i<4;i++)assert(g_sw.installed[i]==0);unsigned ids[4];assert(r58_dock_packages(1600,ids)==4);shot("store-four-installed-HOST-NOT-VM");puts("FOUR_REAL_HTTPS_PACKAGES_INSTALLED_AND_DOCK_PASS");assert(!r59_store_install_available());r58_app_open(3);for(unsigned n=0;n<15000&&!(g_sw.running&8);n++)pump();assert(g_sw.running&8);puts("BATCH_DOWNLOADED_GAME_PROCESS_PASS");for(unsigned i=0;i<3;i++){r58_app_open(i);for(unsigned n=0;n<15000&&!(g_sw.running&(1U<<i));n++)pump();assert(g_sw.running&(1U<<i));}assert(g_sw.running==15);puts("FOUR_DOWNLOADED_APPLICATION_SESSIONS_RUNNING_PASS");nova_store_down(5);nova_sw_down(5);for(unsigned n=0;n<30000&&(g_store.child||g_sw.child);n++){nova_store_reap();nova_sw_reap();usleep(1000);}assert(!g_store.child&&!g_sw.child);return 0;}
