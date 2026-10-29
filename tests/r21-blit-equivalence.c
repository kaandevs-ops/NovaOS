#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NOVA_RENDER_TEST_REFERENCE 1
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
int main(void){
 unsigned int widths[]={1,12,23,24,25,96,640,800};unsigned int heights[]={1,12,23,24,31,400,500};unsigned int cases=0;
 for(unsigned int a=0;a<8;a++)for(unsigned int b=0;b<7;b++)for(unsigned int clipping=0;clipping<4;clipping++){
  unsigned int sw=widths[a],sh=heights[b],bw=clipping==1?(sw>40?sw-19:sw):sw,bh=clipping==1?(sh>40?sh-13:sh):sh;
  unsigned int w=clipping==2?sw/2+16:sw+32,h=clipping==2?sh/2+16:sh+32,pitch=w*4+16;size_t bytes=(size_t)pitch*h;unsigned char*x=malloc(bytes+64),*y=malloc(bytes+64);unsigned int*src=malloc((size_t)bw*bh*4);assert(x&&y&&src);
  for(unsigned int i=0;i<bw*bh;i++)src[i]=0xf1000000U^(i*7919U);memset(x,0x78,bytes+64);memcpy(y,x,bytes+64);
  nova_zero(g_client_buffers,sizeof(g_client_buffers));g_client_buffers[0].used=1;g_client_buffers[0].client_id=3;g_client_buffers[0].buffer_id=7;g_client_buffers[0].map=src;g_client_buffers[0].width=bw;g_client_buffers[0].height=bh;g_client_buffers[0].stride=bw*4;
  struct nova_client_surface s;nova_zero(&s,sizeof(s));s.client_id=3;s.buffer_id=7;s.width=sw;s.height=sh;s.x=clipping==3?-1:8;s.y=8;
  g_test_reference_blit=0;blit_client_buffer(x,w,h,pitch,&s);g_test_reference_blit=1;blit_client_buffer(y,w,h,pitch,&s);assert(!memcmp(x,y,bytes+64));free(x);free(y);free(src);cases++;
 }
 printf("R21_OPAQUE_SPAN_BLIT_REFERENCE_BYTE_EQUIVALENCE_XRGB_CORNERS_CLIPPING_PITCH=%u_CASES_PASS\n",cases);return 0;
}
