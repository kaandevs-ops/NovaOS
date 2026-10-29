#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
static double now_ms(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec*1000.0+t.tv_nsec/1000000.0;}
static int cmp(const void*a,const void*b){double x=*(const double*)a,y=*(const double*)b;return x<y?-1:x>y;}
int main(void){
 unsigned ids[]={0,4,6,3,5};unsigned char*buf=malloc(512*96*4+128);assert(buf);memset(buf,0xa5,512*96*4+128);struct nova_ui_canvas c={buf+64,512,96,512*4};double cold=now_ms();
 for(unsigned i=0;i<5;i++)nova_ui_dock_icon(c,12+(int)i*64,10,ids[i]);cold=now_ms()-cold;assert(nova_dock_cache_builds==5);
 double warm[500];for(unsigned k=0;k<500;k++){double t=now_ms();for(unsigned i=0;i<5;i++)nova_ui_dock_icon(c,12+(int)i*64,10,ids[i]);warm[k]=now_ms()-t;}assert(nova_dock_cache_builds==5);qsort(warm,500,sizeof(double),cmp);
 nova_ui_dock_icon(c,-31,-20,3);nova_ui_dock_icon(c,500,80,5);for(unsigned i=0;i<64;i++)assert(buf[i]==0xa5&&buf[64+512*96*4+i]==0xa5);
 assert(nova_dock_state(1,0,0,0)==1&&nova_dock_state(1,1,1,1)==2);
 printf("{\"scope\":\"host CPU, five icons only, not compositor frame/FPS\",\"cache_builds\":%u,\"cold_ms\":%.6f,\"warm_p50_ms\":%.6f,\"warm_p95_ms\":%.6f,\"cache_bytes\":%zu,\"scratch_bytes\":%zu,\"guard_test\":\"PASS\"}\n",nova_dock_cache_builds,cold,warm[250],warm[475],sizeof nova_dock_cache,sizeof nova_dock_scratch);free(buf);
}
