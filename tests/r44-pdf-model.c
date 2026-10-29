#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/pdf/nova_pdf_sixel.h"
#include "../src/pdf/nova_pdf_protocol.h"
#include "../src/runtime/nova_terminal_sixel.h"
#include "../src/runtime/nova_terminal_grid.h"
static struct nova_term_grid grid;
static struct nova_sixel s;
static void feed(const char*v){nova_sixel_turn(&s);nova_sixel_begin(&s,0,0);for(;*v;v++)nova_sixel_byte(&s,(unsigned char)*v);nova_sixel_end(&s);}
int main(void){
 struct nova_pdf_result h={NOVA_PDF_MAGIC,1,0,0,2,8,6,24,144,8,6,0,0};assert(nova_pdf_result_valid(&h,0,800,600));for(unsigned i=0;i<13;i++){struct nova_pdf_result bad=h;((unsigned*)&bad)[i]=~0U;assert(!nova_pdf_result_valid(&bad,0,800,600));}
 feed("\"1;1;8;6#1;2;100;0;0!8~");assert(s.visible&&!s.bad);assert(nova_sixel_color(&s,7,5)==0xff0000);
 const char*bad[]={"","~","\"1;1;801;6~","\"1;1;8;601~","\"1;1;8;6!9~","\"1;1;8;6#256~","\"1;1;8;6#1;2;101;0;0~","\"1;1;8;6!0~","\"1;1;8;6!8","\"1;1;8;6#1;","\"1;1;8;6!99999999999999~"};for(unsigned i=0;i<sizeof bad/sizeof*bad;i++){feed(bad[i]);assert(s.bad&&!s.visible);}
 nova_sixel_turn(&s);nova_sixel_begin(&s,0,0);nova_sixel_begin(&s,0,0);nova_sixel_begin(&s,0,0);assert(s.bad);
 feed("\"1;1;800;600");assert(s.visible);for(unsigned i=0;i<201;i++){const char*q="$!800~";for(;*q;q++)nova_sixel_byte(&s,*q);}assert(s.bad);
 unsigned char rgb[31*19*3];for(unsigned i=0;i<sizeof rgb;i++)rgb[i]=(unsigned char)(i*73);struct pdf_packet p={malloc(1048576),0,1048576};assert(p.data);assert(!pdf_sixel(&p,rgb,31,19));nova_sixel_turn(&s);nova_sixel_begin(&s,0,0);for(size_t i=3;i<p.size-2;i++)nova_sixel_byte(&s,p.data[i]);nova_sixel_end(&s);assert(s.visible&&!s.bad);
 for(unsigned y=0;y<19;y++)for(unsigned x=0;x<31;x++){unsigned char*v=rgb+(y*31+x)*3;unsigned expected=((v[0]+25)/51*51)<<16|((v[1]+25)/51*51)<<8|((v[2]+25)/51*51);assert(nova_sixel_color(&s,x,y)==expected);}
 /* Exercise the real terminal parser with every stream byte split. */
 nova_term_grid_init(&grid,24,80);grid.graphics=&s;nova_sixel_turn(&s);for(size_t i=0;i<p.size;i++)nova_term_feed(&grid,(unsigned char*)p.data+i,1);assert(s.visible&&!s.bad&&grid.state==0);nova_term_feed(&grid,(const unsigned char*)"\033[2J",4);assert(!s.visible);
 nova_term_feed(&grid,(const unsigned char*)"\033Punknown\033\\OK",14);assert(grid.state==0&&!s.visible);
 p.cap=10;assert(pdf_sixel(&p,rgb,31,19)<0);free(p.data);
 unsigned seed=73;for(unsigned round=0;round<1000;round++){nova_sixel_turn(&s);nova_sixel_begin(&s,0,0);for(unsigned n=0;n<2000;n++){seed=seed*1664525+1013904223;nova_sixel_byte(&s,seed>>24);}nova_sixel_end(&s);assert(!s.visible);}
 puts("R44_PDF_MODEL_PASS: protocol fields, SIXEL roundtrip, caps, malformed and 2M fuzz bytes");return 0;
}
