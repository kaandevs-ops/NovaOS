#include "nova_font.h"

static unsigned char glyph_row(unsigned char c, unsigned int row) {
    if (c >= 'a' && c <= 'z') c = (unsigned char)(c - ('a' - 'A'));
    if (row >= NOVA_FONT_GLYPH_HEIGHT) return 0U;
#define ROWS(a,b,c0,d,e,f,g) do { static const unsigned char r[7]={(a),(b),(c0),(d),(e),(f),(g)}; return r[row]; } while (0)
    switch (c) {
        case 'A': ROWS(14,17,17,31,17,17,17);
        case 'B': ROWS(30,17,17,30,17,17,30);
        case 'C': ROWS(14,17,16,16,16,17,14);
        case 'D': ROWS(30,17,17,17,17,17,30);
        case 'E': ROWS(31,16,16,30,16,16,31);
        case 'F': ROWS(31,16,16,30,16,16,16);
        case 'G': ROWS(14,17,16,23,17,17,15);
        case 'H': ROWS(17,17,17,31,17,17,17);
        case 'I': ROWS(31,4,4,4,4,4,31);
        case 'J': ROWS(7,2,2,2,18,18,12);
        case 'K': ROWS(17,18,20,24,20,18,17);
        case 'L': ROWS(16,16,16,16,16,16,31);
        case 'M': ROWS(17,27,21,21,17,17,17);
        case 'N': ROWS(17,25,21,19,17,17,17);
        case 'O': ROWS(14,17,17,17,17,17,14);
        case 'P': ROWS(30,17,17,30,16,16,16);
        case 'Q': ROWS(14,17,17,17,21,18,13);
        case 'R': ROWS(30,17,17,30,20,18,17);
        case 'S': ROWS(15,16,16,14,1,1,30);
        case 'T': ROWS(31,4,4,4,4,4,4);
        case 'U': ROWS(17,17,17,17,17,17,14);
        case 'V': ROWS(17,17,17,17,17,10,4);
        case 'W': ROWS(17,17,17,21,21,21,10);
        case 'X': ROWS(17,17,10,4,10,17,17);
        case 'Y': ROWS(17,17,10,4,4,4,4);
        case 'Z': ROWS(31,1,2,4,8,16,31);
        case '0': ROWS(14,17,19,21,25,17,14);
        case '1': ROWS(4,12,4,4,4,4,14);
        case '2': ROWS(14,17,1,2,4,8,31);
        case '3': ROWS(30,1,1,14,1,1,30);
        case '4': ROWS(2,6,10,18,31,2,2);
        case '5': ROWS(31,16,16,30,1,1,30);
        case '6': ROWS(14,16,16,30,17,17,14);
        case '7': ROWS(31,1,2,4,8,8,8);
        case '8': ROWS(14,17,17,14,17,17,14);
        case '9': ROWS(14,17,17,15,1,1,14);
        case ':': ROWS(0,4,4,0,4,4,0);
        case '-': ROWS(0,0,0,31,0,0,0);
        case '.': ROWS(0,0,0,0,0,12,12);
        case '/': ROWS(1,2,2,4,8,8,16);
        case '[': ROWS(14,8,8,8,8,8,14);
        case ']': ROWS(14,2,2,2,2,2,14);
        case '+': ROWS(0,4,4,31,4,4,0);
        case '_': ROWS(0,0,0,0,0,0,31);
        case ' ': ROWS(0,0,0,0,0,0,0);
        default:  ROWS(14,17,1,2,4,0,4);
    }
#undef ROWS
}

static unsigned int next_glyph(const char **cursor, unsigned int *replacement) {
    const unsigned char *p=(const unsigned char *)*cursor;
    unsigned int c=(unsigned int)*p;
    *replacement=0U;
    if (!c) return 0U;
    if (c <= NOVA_FONT_UTF8_ASCII_MAX) { *cursor=(const char *)(p+1); return c; }
    unsigned int length=1U;
    if ((c&0xe0U)==0xc0U) length=2U;
    else if ((c&0xf0U)==0xe0U) length=3U;
    else if ((c&0xf8U)==0xf0U) length=4U;
    for (unsigned int i=1U;i<length && p[i];i++) {
        if ((p[i]&0xc0U)!=0x80U) { length=i; break; }
    }
    *cursor=(const char *)(p+length); *replacement=1U; return (unsigned int)'?';
}

struct nova_font_metrics nova_font_measure_text(const char *text, unsigned int scale) {
    struct nova_font_metrics m={0U,0U,0U,0U};
    if (!text || !scale) return m;
    const char *p=text;
    for (;;) {
        unsigned int replacement=0U; unsigned int c=next_glyph(&p,&replacement);
        if (!c) break;
        m.glyphs++; m.replacements+=replacement;
    }
    if (m.glyphs) m.width=((m.glyphs-1U)*NOVA_FONT_GLYPH_ADVANCE+NOVA_FONT_GLYPH_WIDTH)*scale;
    m.height=NOVA_FONT_GLYPH_HEIGHT*scale; return m;
}

static void put_pixel(unsigned char *base, unsigned int width, unsigned int height, unsigned int pitch,
                      int x, int y, unsigned int color,
                      int clip_x, int clip_y, int clip_x1, int clip_y1) {
    if (x<0 || y<0 || x>=(int)width || y>=(int)height) return;
    if (x<clip_x || y<clip_y || x>=clip_x1 || y>=clip_y1) return;
    unsigned int *row=(unsigned int *)(base+(unsigned long)(unsigned int)y*pitch);
    row[(unsigned int)x]=color&0x00ffffffU;
}

unsigned int nova_font_draw_text_xrgb8888(unsigned char *base,
                                           unsigned int width,
                                           unsigned int height,
                                           unsigned int pitch,
                                           int x,
                                           int y,
                                           unsigned int scale,
                                           unsigned int color,
                                           const char *text,
                                           int clip_x,
                                           int clip_y,
                                           unsigned int clip_width,
                                           unsigned int clip_height,
                                           unsigned int *replacements_out) {
    if (replacements_out) *replacements_out=0U;
    if (!base || !text || !scale || !width || !height || !pitch) return 0U;
    int clip_x1=clip_x+(int)clip_width, clip_y1=clip_y+(int)clip_height;
    if (clip_x<0) clip_x=0;
    if (clip_y<0) clip_y=0;
    if (clip_x1>(int)width) clip_x1=(int)width;
    if (clip_y1>(int)height) clip_y1=(int)height;
    if (clip_x>=clip_x1 || clip_y>=clip_y1) return 0U;
    const char *p=text; unsigned int glyphs=0U, replacements=0U; int pen_x=x;
    for (;;) {
        unsigned int replacement=0U; unsigned int c=next_glyph(&p,&replacement);
        if (!c) break;
        replacements+=replacement;
        for (unsigned int gy=0U;gy<NOVA_FONT_GLYPH_HEIGHT;gy++) {
            unsigned char bits=glyph_row((unsigned char)c,gy);
            for (unsigned int gx=0U;gx<NOVA_FONT_GLYPH_WIDTH;gx++) {
                if (!(bits&(1U<<(NOVA_FONT_GLYPH_WIDTH-1U-gx)))) continue;
                for (unsigned int sy=0U;sy<scale;sy++) for (unsigned int sx=0U;sx<scale;sx++)
                    put_pixel(base,width,height,pitch,pen_x+(int)(gx*scale+sx),y+(int)(gy*scale+sy),color,
                              clip_x,clip_y,clip_x1,clip_y1);
            }
        }
        pen_x+=(int)(NOVA_FONT_GLYPH_ADVANCE*scale); glyphs++;
    }
    if (replacements_out) *replacements_out=replacements;
    return glyphs;
}
