#ifndef NOVA_FILES_FONT_H
#define NOVA_FILES_FONT_H
/* Freestanding pre-rasterized Inter glyph alpha atlas; no TTF or runtime parser. */
void nova_files_font_text(unsigned char *base,unsigned int width,unsigned int height,unsigned int pitch,
 int x,int y,unsigned int style,unsigned int color,const char*text,
 int clip_x,int clip_y,unsigned int clip_w,unsigned int clip_h);
void nova_files_font_ellipsis(const char*,char*,unsigned int,unsigned int,unsigned int);
#endif
