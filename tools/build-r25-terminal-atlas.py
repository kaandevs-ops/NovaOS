from pathlib import Path
from PIL import Image,ImageFont,ImageDraw
import hashlib
r=Path(__file__).resolve().parents[1];font_path=r/'assets/fonts/terminal/DejaVuSansMono.ttf'
font=ImageFont.truetype(str(font_path),14)
points=sorted(set([*range(32,127),*range(160,256),0x130,0x131,0x11e,0x11f,0x15e,0x15f,0xfffd,0x2500,0x2502,0x250c,0x2510,0x2514,0x2518]))
glyphs=[]
for cp in points:
 im=Image.new('L',(9,18));ImageDraw.Draw(im).text((0,14),chr(cp),font=font,fill=255,anchor='ls');glyphs.append(list(im.tobytes()))
text='''#ifndef NOVA_TERMINAL_FONT_H
#define NOVA_TERMINAL_FONT_H
/* Generated DejaVu Sans Mono 14px AA coverage, 9x18 cells. No runtime font engine.
 * Source and complete copyright: assets/fonts/terminal. Limited glyph repertoire;
 * unknown codepoints use U+FFFD. No shaping, emoji or full Unicode width support. */
'''
text+='/* TTF SHA256 '+hashlib.sha256(font_path.read_bytes()).hexdigest()+' */\n'
text+='static const unsigned int nova_term_font_codepoints[] = {'+','.join(str(cp) for cp in points)+'};\n'
text+='static const unsigned char nova_term_font_glyphs[][162] = {\n'+',\n'.join('{'+','.join(str(v) for v in glyph)+'}' for glyph in glyphs)+'\n};\n'
text+=r'''
static inline void nova_term_glyph(struct nova_ui_canvas c,int x,int y,unsigned cp,unsigned ink){
 unsigned lo=0,hi=sizeof(nova_term_font_codepoints)/sizeof(nova_term_font_codepoints[0]);
 while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(nova_term_font_codepoints[mid]<cp)lo=mid+1;else hi=mid;}
 if(lo>=sizeof(nova_term_font_codepoints)/sizeof(nova_term_font_codepoints[0])||nova_term_font_codepoints[lo]!=cp)lo=sizeof(nova_term_font_codepoints)/sizeof(nova_term_font_codepoints[0])-1;
 const unsigned char *a=nova_term_font_glyphs[lo];
 for(unsigned yy=0;yy<18;yy++)for(unsigned xx=0;xx<9;xx++)if(a[yy*9+xx])nova_ui_pixel(c,x+(int)xx,y+(int)yy,ink,a[yy*9+xx]);
}
#endif
'''
(r/'src/runtime/nova_terminal_font.h').write_text(text)
print('Built',len(points),'licensed monospace glyphs;',len(points)*162,'atlas bytes')
