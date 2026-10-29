#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define nova_main nova_compositor_unused
#include "../src/services/compositor.c"
#undef nova_main
int main(void){
 unsigned int *a=malloc(400*300*4+32),*b=malloc(400*300*4+32);assert(a&&b);
 int cases[][4]={{40,40,220,170},{-80,-40,200,150},{340,250,180,100},{0,0,800,500},{20,50,24,24},{20,50,23,23}};
 for(unsigned int k=0;k<sizeof(cases)/sizeof(cases[0]);k++){
  for(unsigned int i=0;i<400*300+8;i++)a[i]=b[i]=0x00112233U+(i%79);
  int x=cases[k][0],y=cases[k][1],w=cases[k][2],h=cases[k][3];
  r19_shadow((unsigned char*)a,400,300,1600,x,y,w,h,32,0);
  r19_shadow((unsigned char*)b,400,300,1600,x,y,w,h,32,1);
  if(w>24&&h>24){fill_rect((unsigned char*)a,400,300,1600,x+12,y+12,w-24,h-24,0xffffff);fill_rect((unsigned char*)b,400,300,1600,x+12,y+12,w-24,h-24,0xffffff);}
  assert(!memcmp(a,b,400*300*4+32));
 }
 /* Actual compositor path while a configure keeps an old smaller buffer. */
 nova_zero(g_surfaces,sizeof(g_surfaces));nova_zero(g_client_buffers,sizeof(g_client_buffers));
 struct nova_client_surface*s=&g_surfaces[0];s->used=1;s->committed=1;s->client_id=1;s->surface_id=100;s->buffer_id=1;s->x=40;s->y=40;s->width=128;s->height=100;
 struct nova_client_buffer*buf=&g_client_buffers[0];buf->used=1;buf->client_id=1;buf->buffer_id=1;buf->width=64;buf->height=64;buf->stride=256;buf->bytes=64*64*4;buf->map=calloc(64*64,4);assert(buf->map);
 for(unsigned int i=0;i<400*300;i++)a[i]=b[i]=0x008899aa;
 r19_shadow((unsigned char*)a,400,300,1600,40,40,128,100,22,0);
 draw_client_surfaces((unsigned char*)b,400,300,1600);
 assert(a[90*400+140]==b[90*400+140]&&b[90*400+140]!=0x008899aa);
 free(buf->map);nova_zero(g_client_buffers,sizeof(g_client_buffers));
 free(a);free(b);puts("R18_SHADOW_OPAQUE_OVERDRAW_ELISION_BYTE_EQUIVALENCE_BOUNDS=PASS");return 0;
}
