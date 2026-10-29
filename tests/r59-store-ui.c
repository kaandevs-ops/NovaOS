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
int main(void){g_r2_canvas_width=1600;g_r2_canvas_height=900;r58_layout(1600,900);r58_open(0);settle();assert(g_store.available==8);assert(r58_key(33,NOVA_INPUT_MOD_CTRL));assert(r59_search_focus);assert(r58_key(30,0));assert(r59_query_length==1&&r59_query[0]=='a');assert(r58_key(14,0));assert(!r59_query_length);for(unsigned n=0;n<80;n++)assert(r58_key(30,0));assert(r59_query_length==48);unsigned found[4];assert(r58_rows(found)==0);assert(r58_key(NOVA_KEY_ESC,0));assert(!r59_query_length&&!r59_search_focus);g_sw.tab=1;assert(r58_rows(found)==1&&found[0]==3);g_sw.tab=0;puts("GUI_CATALOG_SEARCH_BOUND_AND_KEYBOARD_PASS");click(g_sw.w-100,70);assert(g_sw.tab==3);click(40,g_sw.h-60);assert(g_store.pending);
 for(unsigned i=0;i<30000&&!g_store.bytes;i++)pump();assert(g_store.bytes&&g_store.bytes<g_store.total);shot("store-downloading-HOST-NOT-VM");click(g_sw.w-80,g_sw.h-60);for(unsigned i=0;i<30000&&g_store.pending;i++)pump();assert(g_store.result==125);settle();assert(g_sw.installed[3]==3);puts("GUI_ACTUAL_PROGRESS_AND_CANCEL_PASS");
 click(40,g_sw.h-60);for(unsigned i=0;i<30000&&(g_store.pending||g_sw.installed[3]!=0);i++)pump();assert(!g_store.pending&&!g_store.result&&g_sw.installed[3]==0);unsigned ids[4];assert(r58_dock_packages(1600,ids)==1&&ids[0]==3);shot("store-installed-HOST-NOT-VM");puts("GUI_HTTPS_INSTALL_AND_DOCK_PASS");
 click(150,g_sw.h-60);for(unsigned i=0;i<10000&&!(g_sw.running&8);i++)pump();assert(g_sw.running&8);for(unsigned i=0;i<300;i++)pump();shot("store-game-HOST-NOT-VM");puts("GUI_DOWNLOADED_GAME_PROCESS_PASS");
 nova_store_down(5);char status[256];nova_store_status(status);assert(strstr(status,"bağlantısı kesildi"));assert(!g_store.ready&&!g_store.available&&!g_store.pending);for(unsigned i=0;i<30000&&g_store.child;i++){nova_store_reap();usleep(1000);}assert(!g_store.child);assert(nova_store_start());assert(!g_store.job&&!g_store.bytes&&!g_store.total&&!g_store.error);for(unsigned i=0;i<30000&&!g_store.ready;i++)pump();assert(g_store.ready&&g_store.available==8);nova_store_status(status);assert(!strstr(status,"Kurulum tamamlandı"));puts("GUI_DISCONNECT_AND_RECONNECT_PASS");nova_store_down(5);nova_sw_down(5);for(unsigned i=0;i<30000&&(g_store.child||g_sw.child);i++){nova_store_reap();nova_sw_reap();usleep(1000);}assert(!g_store.child&&!g_sw.child);puts("R59_LIVE_INTERNET_GUI_PASS");return 0;}
