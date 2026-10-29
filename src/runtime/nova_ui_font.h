#ifndef NOVA_UI_FONT_H
#define NOVA_UI_FONT_H
#include "nova_ui.h"
#include "nova_ui_font_data.h"
enum nova_ui_type { NOVA_UI_BODY=0,NOVA_UI_TITLE=1,NOVA_UI_CAPTION=2,NOVA_UI_HEADING=3 };
static inline unsigned int nova_ui_utf8(const unsigned char*s,unsigned int left,unsigned int*cp){
 if(!left||!s[0])return 0;unsigned int c=s[0],n=1,min=0;
 if(c<128){*cp=c;return 1;}
 if(c>=0xc2&&c<=0xdf){n=2;c&=31;min=128;}
 else if(c>=0xe0&&c<=0xef){n=3;c&=15;min=2048;}
 else if(c>=0xf0&&c<=0xf4){n=4;c&=7;min=65536;}
 else{*cp='?';return 1;}
 if(n>left){*cp='?';return 1;}
 for(unsigned int i=1;i<n;i++){if((s[i]&192U)!=128U){*cp='?';return 1;}c=(c<<6)|(s[i]&63U);}
 if(c<min||c>0x10ffffU||(c>=0xd800U&&c<=0xdfffU)){*cp='?';return 1;}
 *cp=c;return n;
}
static inline unsigned int nova_ui_glyph_index(unsigned int cp){
 unsigned int lo=0,hi=NOVA_UI_GLYPH_COUNT;
 while(lo<hi){unsigned int m=(lo+hi)/2;if(nova_ui_faces[0].glyphs[m].cp<cp)lo=m+1;else hi=m;}
 if(lo<NOVA_UI_GLYPH_COUNT&&nova_ui_faces[0].glyphs[lo].cp==cp)return lo;
 return (unsigned int)('?'-32);
}
static inline const struct nova_ui_face*nova_ui_face_for(unsigned int role,unsigned int scale){
 if(role>3)role=0;return &nova_ui_faces[role+(scale==2?4U:0U)];
}
static inline int nova_ui_kerning(const struct nova_ui_face*f,unsigned int prev,unsigned int next){
 for(unsigned int i=0;i<f->nkern;i++)if(f->kern[i].left==prev&&f->kern[i].right==next)return f->kern[i].adjust;return 0;
}
static inline unsigned int nova_ui_measure(const char*text,unsigned int role,unsigned int scale){
 if(!text)return 0;const struct nova_ui_face*f=nova_ui_face_for(role,scale);int pen=0;unsigned int pos=0,prev=~0U;
 while(pos<512U&&text[pos]){unsigned int cp,n=nova_ui_utf8((const unsigned char*)text+pos,512U-pos,&cp);if(!n)break;
  unsigned int g=nova_ui_glyph_index(cp);if(prev!=~0U)pen+=nova_ui_kerning(f,prev,g);pen+=f->glyphs[g].advance;prev=g;pos+=n;}
 return pen>0?(unsigned int)(pen+63)/64U:0;
}
static inline void nova_ui_text(struct nova_ui_canvas c,int x,int baseline,unsigned int role,unsigned int scale,
 unsigned int ink,const char*text,int cx,int cy,unsigned int cw,unsigned int ch){
 if(!c.pixels||!text||!c.width||!c.height||c.pitch<c.width*4U||c.pitch%4U)return;
 const struct nova_ui_face*f=nova_ui_face_for(role,scale);int pen=x*64;unsigned int pos=0,prev=~0U;
 int left=cx<0?0:cx,top=cy<0?0:cy,right=cx+(int)cw,bottom=cy+(int)ch;
 if(right>(int)c.width)right=(int)c.width;if(bottom>(int)c.height)bottom=(int)c.height;
 while(pos<512U&&text[pos]){
  unsigned int cp,n=nova_ui_utf8((const unsigned char*)text+pos,512U-pos,&cp);if(!n)break;
  unsigned int gi=nova_ui_glyph_index(cp);const struct nova_ui_glyph*g=&f->glyphs[gi];
  if(prev!=~0U)pen+=nova_ui_kerning(f,prev,gi);int gx=(pen+32)/64+g->x,gy=baseline+g->y;
  if(gx>=right)break;
  for(unsigned int j=0;j<g->h;j++)for(unsigned int k=0;k<g->w;k++){
   int px=gx+(int)k,py=gy+(int)j;if(px<left||px>=right||py<top||py>=bottom)continue;
   unsigned int a=f->alpha[g->offset+j*g->w+k];if(a)nova_ui_pixel(c,px,py,ink,a);
  }
  pen+=g->advance;prev=gi;pos+=n;
 }
}
static inline void nova_ui_ellipsis(const char*src,char*out,unsigned int cap,unsigned int role,unsigned int max_width){
 if(!out||!cap)return;out[0]=0;if(!src)return;
 unsigned int bytes=0;while(bytes<512U&&src[bytes])bytes++;
 unsigned int ends[128],count=0,pos=0,nout=0;int overflow=0;
 while(pos<bytes&&count<128){unsigned int cp,n=nova_ui_utf8((const unsigned char*)src+pos,bytes-pos,&cp);if(!n)break;
  unsigned int gi=nova_ui_glyph_index(cp);int supported=nova_ui_faces[0].glyphs[gi].cp==cp;
  unsigned int take=supported?n:1U;if(nout+take>=cap){overflow=1;break;}
  for(unsigned int j=0;j<take;j++)out[nout++]=supported?src[pos+j]:'?';pos+=n;out[nout]=0;ends[count++]=nout;
  if(nova_ui_measure(out,role,1)>max_width){nout=count>1?ends[count-2]:0;count--;out[nout]=0;overflow=1;break;}
 }
 if(pos<bytes)overflow=1;
 if(overflow){
  unsigned int ew=nova_ui_measure("\xe2\x80\xa6",role,1);
  while(count&&(nout+4U>cap||nova_ui_measure(out,role,1)+ew>max_width)){count--;nout=count?ends[count-1]:0;out[nout]=0;}
  if(nout+4U<=cap&&ew<=max_width){out[nout++]=(char)0xe2;out[nout++]=(char)0x80;out[nout++]=(char)0xa6;out[nout]=0;}
 }
}
#endif
