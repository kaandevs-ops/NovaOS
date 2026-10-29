#include <stdio.h>
#include <string.h>
#include "nova_font.h"

static unsigned int pixels[96U*32U];
static int pixel(unsigned int x,unsigned int y,unsigned int c){ return pixels[y*96U+x]==c; }
int main(void){
    memset(pixels,0,sizeof(pixels));
    struct nova_font_metrics m=nova_font_measure_text("NOVAOS",2U);
    if(m.width!=70U || m.height!=14U || m.glyphs!=6U || m.replacements!=0U) return 10;
    unsigned int repl=99U;
    unsigned int g=nova_font_draw_text_xrgb8888((unsigned char*)pixels,96U,32U,96U*4U,2,3,2U,0x00ffffffU,"N",0,0,96U,32U,&repl);
    if(g!=1U || repl!=0U) return 11;
    if(!pixel(2U,3U,0x00ffffffU) || !pixel(10U,3U,0x00ffffffU) || !pixel(6U,3U,0U)) return 12;
    memset(pixels,0,sizeof(pixels)); repl=0U;
    g=nova_font_draw_text_xrgb8888((unsigned char*)pixels,96U,32U,96U*4U,0,0,1U,0x00112233U,"A\xc3\xa9",0,0,6U,7U,&repl);
    if(g!=2U || repl!=1U) return 13;
    /* Second glyph starts outside the six-pixel clip, proving clipping is bounded. */
    for(unsigned int y=0;y<7U;y++) for(unsigned int x=6U;x<96U;x++) if(pixels[y*96U+x]) return 14;
    puts("Stage-5A Phase-8 bitmap-font metrics/ASCII-UTF8-fallback/clipping runtime unit: PASS");
    return 0;
}
