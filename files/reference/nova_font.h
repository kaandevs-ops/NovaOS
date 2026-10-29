#ifndef NOVA_FONT_H
#define NOVA_FONT_H

#define NOVA_FONT_GLYPH_WIDTH 5U
#define NOVA_FONT_GLYPH_HEIGHT 7U
#define NOVA_FONT_GLYPH_ADVANCE 6U
#define NOVA_FONT_UTF8_ASCII_MAX 0x7fU

struct nova_font_metrics {
    unsigned int width;
    unsigned int height;
    unsigned int glyphs;
    unsigned int replacements;
};

struct nova_font_metrics nova_font_measure_text(const char *text, unsigned int scale);
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
                                           unsigned int *replacements_out);

#endif
