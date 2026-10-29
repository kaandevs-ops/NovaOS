#ifndef NOVA_WALLPAPER_CACHE_H
#define NOVA_WALLPAPER_CACHE_H
/* Bounded CPU RGB565 conversion cache. No allocation, file access or frame copy.
 * The immutable palette preserves the existing Nova photo tone exactly. */
#define NOVA_WALLPAPER_MAP_MAX 4096U
struct nova_wallpaper_cache {
 unsigned int palette[65536];
 unsigned short columns[NOVA_WALLPAPER_MAP_MAX];
 unsigned int ready,width,source_width;
 unsigned long long frames,palette_builds,map_builds,fallbacks;
};
static unsigned int nova_wallpaper_color(unsigned int raw){
 unsigned int red=((raw>>11)&31U)*255U/31U;
 unsigned int green=((raw>>5)&63U)*255U/63U;
 unsigned int blue=(raw&31U)*255U/31U;
 return (((red*215U+8U*40U)/255U)<<16)|(((green*215U+7U*40U)/255U)<<8)|((blue*215U+9U*40U)/255U);
}
static int nova_wallpaper_cached(struct nova_wallpaper_cache*c,const unsigned char*src,
 unsigned long src_bytes,unsigned int sw,unsigned int sh,unsigned char*dst,
 unsigned int w,unsigned int h,unsigned int pitch){
 if(!c||!src||!dst||!sw||!sh||!w||!h||sw>NOVA_WALLPAPER_MAP_MAX||sh>NOVA_WALLPAPER_MAP_MAX||w>NOVA_WALLPAPER_MAP_MAX||h>NOVA_WALLPAPER_MAP_MAX||pitch<w*4U||src_bytes<(unsigned long)sw*sh*2UL)return 0;
 if(!c->ready){for(unsigned int i=0;i<65536U;i++)c->palette[i]=nova_wallpaper_color(i);c->ready=1;c->palette_builds++;}
 if(c->width!=w||c->source_width!=sw){for(unsigned int x=0;x<w;x++)c->columns[x]=(unsigned short)((unsigned long long)x*sw/w);c->width=w;c->source_width=sw;c->map_builds++;}
 for(unsigned int y=0;y<h;y++){
  const unsigned char*line=src+(unsigned long)((unsigned long long)y*sh/h)*sw*2UL;
  unsigned int*row=(unsigned int*)(dst+(unsigned long)y*pitch);
  for(unsigned int x=0;x<w;x++){unsigned int j=(unsigned int)c->columns[x]*2U;unsigned int raw=(unsigned int)line[j]|((unsigned int)line[j+1U]<<8);row[x]=c->palette[raw];}
 }
 c->frames++;return 1;
}
#endif
