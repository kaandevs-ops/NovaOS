#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "nova_wallpaper_cache.h"
static unsigned int reference(unsigned int raw){
 unsigned int red=((raw>>11)&31U)*255U/31U,green=((raw>>5)&63U)*255U/63U,blue=(raw&31U)*255U/31U;
 unsigned int c=(red<<16)|(green<<8)|blue,to=0x00080709U;
 unsigned int rr=((((c>>16)&255U)*215U+((to>>16)&255U)*40U)/255U);
 unsigned int gg=((((c>>8)&255U)*215U+((to>>8)&255U)*40U)/255U);
 unsigned int bb=(((c&255U)*215U+(to&255U)*40U)/255U);
 return (rr<<16)|(gg<<8)|bb;
}
int main(void){
 size_t cb=sizeof(struct nova_wallpaper_cache);unsigned char*memory=malloc(cb+64);assert(memory);memset(memory,0x61,cb+64);struct nova_wallpaper_cache*c=(void*)(memory+32);memset(c,0,cb);
 unsigned char*src=malloc(65536U*2U);assert(src);for(unsigned int raw=0;raw<65536U;raw++){src[raw*2]=(unsigned char)raw;src[raw*2+1]=(unsigned char)(raw>>8);assert(nova_wallpaper_color(raw)==reference(raw));}
 unsigned int sizes[][2]={{256,256},{640,400},{1280,800},{1600,900},{1920,1080},{4096,2},{4,4096},{640,400}};
 for(unsigned int i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++){
  unsigned int w=sizes[i][0],h=sizes[i][1],pitch=w*4U+16U;size_t bytes=(size_t)pitch*h;unsigned char*dst=malloc(bytes+64);assert(dst);memset(dst,0x61,bytes+64);
  assert(nova_wallpaper_cached(c,src,65536U*2U,256,256,dst+32,w,h,pitch));
  for(unsigned int y=0;y<h;y++){unsigned int*row=(void*)(dst+32+(size_t)y*pitch);unsigned int sy=(unsigned long long)y*256U/h;for(unsigned int x=0;x<w;x++){unsigned int sx=(unsigned long long)x*256U/w;assert(row[x]==reference(sy*256U+sx));}for(unsigned int k=w*4;k<pitch;k++)assert(dst[32+(size_t)y*pitch+k]==0x61);}
  for(unsigned int k=0;k<32;k++)assert(dst[k]==0x61&&dst[bytes+32+k]==0x61);free(dst);
 }
 assert(c->palette_builds==1&&c->frames==8&&c->map_builds==8);
 unsigned int*changed=malloc(640U*400U*4U);assert(changed);assert(nova_wallpaper_cached(c,src,65536U*2U,128,512,(unsigned char*)changed,640,400,640*4));for(unsigned int y=0;y<400;y++)for(unsigned int x=0;x<640;x++){unsigned int sy=(unsigned long long)y*512/400,sx=(unsigned long long)x*128/640;assert(changed[y*640+x]==reference(sy*128+sx));}free(changed);assert(c->map_builds==9&&c->palette_builds==1);
 unsigned char pixel[4]={1,2,3,4};unsigned long long frames=c->frames;
 assert(!nova_wallpaper_cached(c,src,1,256,256,pixel,1,1,4));
 assert(!nova_wallpaper_cached(c,src,65536*2,256,256,pixel,4097,1,4097*4));
 assert(!nova_wallpaper_cached(c,src,65536*2,256,256,pixel,1,4097,4));
 assert(!nova_wallpaper_cached(c,src,65536*2,256,256,pixel,1,1,3));
 assert(!nova_wallpaper_cached(c,src,65536*2,0,256,pixel,1,1,4));
 assert(!nova_wallpaper_cached(c,0,65536*2,256,256,pixel,1,1,4));
 assert(!nova_wallpaper_cached(0,src,65536*2,256,256,pixel,1,1,4));
 assert(pixel[0]==1&&pixel[1]==2&&pixel[2]==3&&pixel[3]==4&&c->frames==frames);
 for(unsigned int k=0;k<32;k++)assert(memory[k]==0x61&&memory[cb+32+k]==0x61);
 printf("R24_WALLPAPER_EXHAUSTIVE_65536_COLORS_8_SIZES_PADDING_CACHE_REBUILD_INVALID_BOUNDS=%zu_BYTES_PASS\n",cb);free(src);free(memory);return 0;
}
