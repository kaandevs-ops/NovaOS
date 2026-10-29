#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "../src/compat/nova_x11_pixels.c"
static double tick(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec+t.tv_nsec/1e9;}
int main(void){unsigned w=1024,h=650;size_t bytes=(size_t)w*h*4;char*data=malloc((w*4+16)*h);unsigned char*a=malloc(bytes+16),*b=malloc(bytes+16);assert(data&&a&&b);
 unsigned state=7;for(size_t n=0;n<(w*4+16)*h;n++){state=state*1664525U+1013904223U;data[n]=(char)(state>>24);}
 XImage image={.width=(int)w,.height=(int)h,.format=ZPixmap,.data=data,.byte_order=LSBFirst,.bitmap_unit=32,.bitmap_bit_order=LSBFirst,.bitmap_pad=32,.depth=24,.bytes_per_line=(int)w*4+16,.bits_per_pixel=32,.red_mask=0xff0000,.green_mask=0xff00,.blue_mask=0xff};assert(XInitImage(&image));
 for(int mode=0;mode<3;mode++){
  if(mode==1){image.red_mask=0xff;image.blue_mask=0xff0000;}if(mode==2)image.byte_order=MSBFirst;
  assert(XInitImage(&image));memset(a,0xa5,bytes+16);memset(b,0xa5,bytes+16);copy_image(&image,w,h,a,0);copy_image(&image,w,h,b,1);assert(!memcmp(a,b,bytes+16));for(unsigned n=0;n<16;n++)assert(a[bytes+n]==0xa5);
 }
 image.byte_order=LSBFirst;image.red_mask=0xff0000;image.blue_mask=0xff;assert(XInitImage(&image));
 double t=tick();for(int i=0;i<30;i++)copy_image(&image,w,h,a,0);double fast=(tick()-t)/30;
 t=tick();for(int i=0;i<30;i++)copy_image(&image,w,h,b,1);double reference=(tick()-t)/30;assert(!memcmp(a,b,bytes));
 printf("PASS pixel equivalence, padded rows, masks/endian fallback, output bounds; host unpack-only fast_ms=%.3f reference_ms=%.3f (not VM/FPS)\n",fast*1000,reference*1000);
 free(data);free(a);free(b);return 0;}
