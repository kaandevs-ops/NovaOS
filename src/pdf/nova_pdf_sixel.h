#ifndef NOVA_PDF_SIXEL_H
#define NOVA_PDF_SIXEL_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Complete packets are built before publication; a failed budget never emits
 * half a DCS. Fixed 6x6x6 palette, white background, no external assets. */
struct pdf_packet {char *data;size_t size,cap;};
static int pdf_put(struct pdf_packet*p,const char*s,size_t n){if(n>p->cap-p->size)return -1;memcpy(p->data+p->size,s,n);p->size+=n;return 0;}
static int pdf_run(struct pdf_packet*p,unsigned bits,unsigned n){char b[32];int k=n>3?snprintf(b,sizeof b,"!%u%c",n,(int)(63+bits)):0;if(n>3)return pdf_put(p,b,(size_t)k);while(n--)if(pdf_put(p,(char[]){(char)(63+bits)},1))return -1;return 0;}
static int pdf_sixel(struct pdf_packet*p,const unsigned char*rgb,unsigned w,unsigned h){
 if(!w||w>800||!h||h>600)return -1;char b[64];p->size=0;int k=snprintf(b,sizeof b,"\033Pq\"1;1;%u;%u",w,h);if(pdf_put(p,b,(size_t)k))return -1;
 for(unsigned c=0;c<216;c++){k=snprintf(b,sizeof b,"#%u;2;%u;%u;%u",c,(c/36)*20,((c/6)%6)*20,(c%6)*20);if(pdf_put(p,b,(size_t)k))return -1;}
 unsigned char masks[216][800];
 for(unsigned y=0;y<h;y+=6){memset(masks,0,sizeof masks);unsigned used[216]={0};
  for(unsigned bit=0;bit<6&&y+bit<h;bit++)for(unsigned x=0;x<w;x++){const unsigned char*v=rgb+((size_t)(y+bit)*w+x)*3;unsigned c=((v[0]+25)/51)*36+((v[1]+25)/51)*6+(v[2]+25)/51;if(c!=215){masks[c][x]|=(unsigned char)(1U<<bit);used[c]=1;}}
  for(unsigned c=0;c<215;c++)if(used[c]){k=snprintf(b,sizeof b,"#%u",c);if(pdf_put(p,b,(size_t)k))return -1;unsigned end=w;while(end&&!masks[c][end-1])end--;for(unsigned x=0;x<end;){unsigned n=1;while(x+n<end&&masks[c][x+n]==masks[c][x])n++;if(pdf_run(p,masks[c][x],n))return -1;x+=n;}if(pdf_put(p,"$",1))return -1;}
  if(y+6<h&&pdf_put(p,"-",1))return -1;
 }return pdf_put(p,"\033\\",2);
}
#endif
