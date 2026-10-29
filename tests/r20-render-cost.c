#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
static double stamp(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1e6+t.tv_nsec/1000.0;}
int main(void){
 unsigned int w=1600,h=900,pitch=w*4;unsigned char*b=calloc(h,pitch);if(!b)return 1;struct nova_ui_canvas c={b,w,h,pitch};double a,txt,icon,shadow;volatile unsigned long check=0;
 nova_ui_text(c,20,40,NOVA_UI_BODY,1,0x222c35,"Notlar-İstanbul Çalışmalar",0,0,w,h);nova_ui_app_icon(c,20,50,48,3);r19_shadow(b,w,h,pitch,200,120,800,500,32,1);
 a=stamp();for(int n=0;n<10000;n++)nova_ui_text(c,20,40,NOVA_UI_BODY,1,0x222c35,"Notlar-İstanbul Çalışmalar",0,0,w,h);txt=(stamp()-a)/10000;
 a=stamp();for(int n=0;n<5000;n++)nova_ui_app_icon(c,20,50,48,3);icon=(stamp()-a)/5000;
 a=stamp();for(int n=0;n<1000;n++)r19_shadow(b,w,h,pitch,200,120,800,500,32,1);shadow=(stamp()-a)/1000;
 for(unsigned long k=0;k<(unsigned long)pitch*h;k++)check+=b[k];printf("{\"scope\":\"warm host CPU microbenchmark; no DRM, no runtime rasterizer, no blur\",\"text_26chars_us\":%.3f,\"app_icon_48_us\":%.3f,\"cached_shadow_800x500_us\":%.3f,\"checksum\":%lu}\n",txt,icon,shadow,check);free(b);return 0;
}
