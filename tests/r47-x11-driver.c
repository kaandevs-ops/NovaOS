#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "nova_x11_pixels.h"
int main(int n,char**a){if(n<3)return 2;struct nova_x11*c=nova_x11_open(a[1]);if(!c)return 3;unsigned w,h;if(nova_x11_size(c,&w,&h))return 4;
 if(!strcmp(a[2],"close")){int rc=nova_x11_request_close(c);nova_x11_close(c);return rc?1:0;}
 if(!strcmp(a[2],"key")&&n==5){int rc=nova_x11_key(c,(unsigned)atoi(a[3]),atoi(a[4]));nova_x11_close(c);return rc?1:0;}
 if(!strcmp(a[2],"click")&&n==5){int x=atoi(a[3]),y=atoi(a[4]);int rc=nova_x11_pointer(c,x,y,1,1);rc|=nova_x11_pointer(c,x,y,1,0);nova_x11_close(c);return rc?1:0;}
 if(strcmp(a[2],"capture")||n!=4)return 2;size_t bytes=(size_t)w*h*4;uint32_t*b=malloc(bytes);if(!b||nova_x11_pixels(c,b,bytes))return 5;FILE*f=fopen(a[3],"wb");if(!f)return 6;fprintf(f,"P6\n%u %u\n255\n",w,h);for(size_t i=0;i<(size_t)w*h;i++){unsigned char rgb[]={b[i]>>16,b[i]>>8,b[i]};if(fwrite(rgb,1,3,f)!=3)return 7;}if(fclose(f))return 8;free(b);nova_x11_close(c);return 0;}
