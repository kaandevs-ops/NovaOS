#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
int main(void){
 struct nova_sw_packet p={.magic=NOVA_SW_MAGIC,.version=1,.op=NOVA_SW_LIST,.tag=1};
 assert(nova_sw_valid(&p,32,1));p.index=4;assert(!nova_sw_valid(&p,32,1));p.index=0;p.reserved=1;assert(!nova_sw_valid(&p,32,1));p.reserved=0;p.op=99;assert(!nova_sw_valid(&p,32,1));
 p.op=NOVA_SW_INPUT;p.length=513;assert(!nova_sw_valid(&p,545,1));p.length=1;assert(nova_sw_valid(&p,33,1));assert(!nova_sw_valid(&p,32,1));
 unsigned sizes[][2]={{360,400},{640,480},{1280,800},{1600,900},{1920,1080}};
 for(unsigned n=0;n<5;n++){unsigned w=sizes[n][0],h=sizes[n][1];size_t bytes=(size_t)w*h*4;unsigned char*b=malloc(bytes+64);memset(b,0,bytes);memset(b+bytes,0xa5,64);g_sw.w=0;g_sw.visible=1;g_sw.ready=1;g_sw.installed[0]=0;g_sw.installed[1]=3;g_sw.installed[2]=3;g_sw.installed[3]=0;g_r2_canvas_width=w;g_r2_canvas_height=h;r58_draw(b,w,h,w*4);for(unsigned j=0;j<64;j++)assert(b[bytes+j]==0xa5);assert(g_sw.x>=0&&g_sw.x+g_sw.w<=(int)w);assert(g_sw.y+g_sw.h<=(int)h);assert(r58_at(g_sw.x+20,g_sw.y+20));assert(!r58_at(0,0));if(n==3){nova_r2_dock(b,w,h,w*4);FILE*f=fopen("evidence/r58/software-host.ppm","wb");assert(f);fprintf(f,"P6\n%u %u\n255\n",w,h);for(size_t z=0;z<bytes;z+=4){fputc(b[z+2],f);fputc(b[z+1],f);fputc(b[z],f);}fclose(f);}free(b);}
 g_sw.ready=1;g_sw.installed[0]=0;g_sw.installed[1]=3;g_sw.installed[2]=3;g_sw.installed[3]=0;
 unsigned ids[4];assert(r58_dock_packages(1600,ids)==2&&ids[0]==0&&ids[1]==3);g_sw.ready=0;assert(!r58_dock_packages(1600,ids));
 puts("R58_PROTOCOL_AND_RENDER_BOUNDS_PASS");return 0;
}
