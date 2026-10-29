#ifndef NOVA_UI_H
#define NOVA_UI_H
/* Nova native UI v1: bounded XRGB raster primitives, no allocation or authority.
 * Coordinates are logical pixels. AA coverage is evaluated only at the edge.
 * Inter atlases and Lucide assets retain their original independent licences. */
#define NOVA_UI_PAPER 0x00fafbfdU
#define NOVA_UI_INK 0x00263042U
#define NOVA_UI_MUTED 0x00606c7bU
#define NOVA_UI_ACCENT 0x00376cbaU
#define NOVA_UI_SELECTION 0x00e2ecfaU
#define NOVA_UI_LINE 0x00e4e8eeU
#define NOVA_UI_PANEL_HEIGHT 38U
#define NOVA_UI_TASK_BASE 152
#define NOVA_UI_TASK_PITCH 112
#define NOVA_UI_DOCK_PITCH 64U
#define NOVA_UI_SPACE 8
#define NOVA_UI_ROW 40
#define NOVA_UI_RADIUS 8
struct nova_ui_canvas { unsigned char *pixels; unsigned int width,height,pitch; };
static inline void nova_ui_pixel(struct nova_ui_canvas c,int x,int y,unsigned int ink,unsigned int a){
 if(x<0||y<0||(unsigned int)x>=c.width||(unsigned int)y>=c.height||!a)return;
 unsigned int *p=(unsigned int*)(c.pixels+(unsigned long)(unsigned int)y*c.pitch)+(unsigned int)x;
 /* Fully opaque coverage is an exact RGB store, not a read/modify/blend. */
 if(a==255U){*p=ink&0x00ffffffU;return;}
 unsigned int b=*p,inv=255-a;
 *p=((((b>>16&255)*inv+(ink>>16&255)*a+127)/255)<<16)|
 ((((b>>8&255)*inv+(ink>>8&255)*a+127)/255)<<8)|((b&255)*inv+(ink&255)*a+127)/255;
}
static inline void nova_ui_round(struct nova_ui_canvas c,int x,int y,int w,int h,int radius,unsigned int ink){
 if(w<=0||h<=0)return;
 int r=radius;if(r>w/2)r=w/2;if(r>h/2)r=h/2;if(r<0)r=0;
 int l=x<0?0:x,t=y<0?0:y,rr=x+w>(int)c.width?(int)c.width:x+w,bb=y+h>(int)c.height?(int)c.height:y+h;
 for(int j=t;j<bb;j++)for(int i=l;i<rr;i++){
  int dx=i-x<r?r*4-(i-x)*4-2:i-x>=w-r?(i-x-w+r)*4+2:0;
  int dy=j-y<r?r*4-(j-y)*4-2:j-y>=h-r?(j-y-h+r)*4+2:0;
  unsigned int a=255;
  if(dx&&dy){int d=dx*dx+dy*dy,inner=(r*4-2)*(r*4-2),outer=(r*4+2)*(r*4+2);
   if(d>=outer)continue;if(d>inner)a=(unsigned int)((outer-d)*255/(outer-inner));}
  nova_ui_pixel(c,i,j,ink,a);
 }
}
static inline void nova_ui_folder(struct nova_ui_canvas c,int x,int y,int size,unsigned int ink){
 nova_ui_round(c,x+size/12,y+size/6,size*5/12,size/4,size/12,ink);
 nova_ui_round(c,x,y+size/3,size,size*2/3,size/8,ink);
 nova_ui_round(c,x+size/12,y+size*5/12,size*5/6,size/12,size/24,0x00ffffffU);
}
/* Desktop identity art: layered folder silhouette, inset paper and restrained depth.
 * UI glyphs retain their smaller, simpler folder geometry. No runtime assets required. */
static inline void nova_ui_desktop_folder(struct nova_ui_canvas c,int x,int y,int size,unsigned accent,int selected){
 if(size<16||size>128)return;
 nova_ui_round(c,x+2,y+size/3+3,size-4,size*3/5,size/8,0x00233444);
 nova_ui_round(c,x+2,y+size/6,size*5/12,size/3,size/12,accent);
 nova_ui_round(c,x+1,y+size/4,size-2,size*2/3,size/9,accent);
 nova_ui_round(c,x+size/9,y+size/3,size*7/9,size/3,size/24,0x00edf5ff);
 nova_ui_round(c,x,y+size*2/5,size,size*3/5,size/8,selected?0x009bcfff:0x008bbde5);
 nova_ui_round(c,x+size/10,y+size*2/5+2,size*4/5,2,1,0x00cde8ff);
}
static inline void nova_ui_document(struct nova_ui_canvas c,int x,int y,int size,unsigned int ink){
 nova_ui_round(c,x+size/8,y,size*3/4,size,size/8,ink);
 nova_ui_round(c,x+size/3,y+size/3,size/3,1,0,0x00ffffffU);
 nova_ui_round(c,x+size/3,y+size/2,size/3,1,0,0x00ffffffU);
}
/* Integer subpixel line coverage, designed for small native control glyphs. */
static inline void nova_ui_line(struct nova_ui_canvas c,int x0,int y0,int x1,int y1,unsigned int ink){
 int dx=x1-x0,dy=y1-y0,length=dx*dx+dy*dy;if(!length)return;
 int l=x0<x1?x0:x1,r=x0>x1?x0:x1,t=y0<y1?y0:y1,b=y0>y1?y0:y1;
 for(int y=t-1;y<=b+1;y++)for(int x=l-1;x<=r+1;x++){
  int dot=(x-x0)*dx+(y-y0)*dy;if(dot<0||dot>length)continue;
  int cross=(x-x0)*dy-(y-y0)*dx,dist=cross*cross*256/length;
  if(dist<256)nova_ui_pixel(c,x,y,ink,(unsigned int)(256-dist)*255/256);
 }
}

struct nova_ui_palette {unsigned int paper,sidebar,toolbar,ink,muted,line,selection,hover,accent;};
static inline struct nova_ui_palette nova_ui_palette_for(unsigned int theme){
 if(theme==1U){struct nova_ui_palette p={0x00202732,0x0028303d,0x00242c38,0x00edf2f8,0x00acbacd,0x003b4655,0x00374761,0x002e3a4d,0x008db8f3};return p;}
 if(theme==0U){struct nova_ui_palette p={0x00ffffff,0x00f1f3f7,0x00f8f9fb,0x00222b3c,0x005b6879,0x00e7ebf1,0x00dfeafb,0x00f0f5fc,0x003262ad};return p;}
 struct nova_ui_palette p={0x00fdfdfb,0x00edf1f0,0x00f4f7f5,0x00232e35,0x0057646c,0x00e2e8e5,0x00d9e9e5,0x00eff5f2,0x00326259};return p;
}
/* App identity art differs from small monochrome control glyphs. Original NovaOS geometry. */
static inline void nova_ui_app_icon(struct nova_ui_canvas c,int x,int y,int size,unsigned int app){
 unsigned int base=app==3?0x003e77be:app==1?0x004d678e:app==2?0x003a8e85:app==4?0x006c66a0:0x005e78a2;
 nova_ui_round(c,x,y,size,size,size/4,0x00d8e2ec);
 for(int j=1;j<size-1;j++)for(int i=1;i<size-1;i++){
  int rad=size/4,dx=i<rad?rad*4-i*4-2:i>=size-rad?(i-size+rad)*4+2:0;
  int dy=j<rad?rad*4-j*4-2:j>=size-rad?(j-size+rad)*4+2:0;unsigned int alpha=255;
  if(dx&&dy){int d=dx*dx+dy*dy,inner=(rad*4-2)*(rad*4-2),outer=(rad*4+2)*(rad*4+2);if(d>=outer)continue;if(d>inner)alpha=(unsigned int)((outer-d)*255/(outer-inner));}
  unsigned int a=(unsigned int)(j*32/(size?size:1)),col=base+((a/4)<<16)+((a/5)<<8)+a/5;
  nova_ui_pixel(c,x+i,y+j,col,alpha);
 }
 if(app==3){
  nova_ui_folder(c,x+size/5,y+size/4,size*3/5,0x00e6f1ff);
  nova_ui_round(c,x+size/4,y+size*3/5,size/2,size/10,size/20,0x00b4d4f7);
 }else if(app==1){
  nova_ui_document(c,x+size/3,y+size/5,size/2,0x00e6effa);
  nova_ui_line(c,x+size/4,y+size*3/4,x+size/2,y+size*3/4,0xffffff);
 }else if(app==2){
  nova_ui_round(c,x+size/4,y+size/4,size/2,size/2,size/8,0x00e7f5f1);
  nova_ui_round(c,x+size/3,y+size/3,size/3,size/3,size/12,base);
 }else if(app==5){
  /* Original web identity, not a copied browser trademark. */
  nova_ui_round(c,x+size/5,y+size/5,size*3/5,size*3/5,size*3/10,0x00e8f5ff);
  nova_ui_round(c,x+size/5+2,y+size/5+2,size*3/5-4,size*3/5-4,size*3/10-2,0x003b7eaa);
  nova_ui_line(c,x+size/5+3,y+size/2,x+size*4/5-3,y+size/2,0x00e8f5ff);
  nova_ui_line(c,x+size/2,y+size/5+2,x+size/2,y+size*4/5-2,0x00e8f5ff);
  nova_ui_line(c,x+size/3,y+size/3,x+size*2/3,y+size/3,0x00e8f5ff);
  nova_ui_line(c,x+size/3,y+size*2/3,x+size*2/3,y+size*2/3,0x00e8f5ff);
 }else if(app==4){
  for(int j=0;j<3;j++){int yy=y+size/3+j*size/6;nova_ui_line(c,x+size/4,yy,x+size*3/4,yy,0xf5f3ff);nova_ui_round(c,x+size/3+(j%2)*size/4,yy-size/16,size/8,size/8,size/16,0xf5f3ff);}
 }else{
  for(int j=0;j<2;j++)for(int i=0;i<2;i++)nova_ui_round(c,x+size/4+i*size/3,y+size/4+j*size/3,size/5,size/5,size/12,0xf5f6fb);
 }
}
/* Shared interaction state for native toolbar and future applications. */
enum nova_ui_state { NOVA_UI_DISABLED=1U,NOVA_UI_HOVER=2U,NOVA_UI_PRESSED=4U,NOVA_UI_FOCUSED=8U,NOVA_UI_SELECTED=16U };
static inline unsigned int nova_ui_button(struct nova_ui_canvas c,int x,int y,int w,int h,struct nova_ui_palette p,unsigned int state){
 if(state&NOVA_UI_DISABLED)state=NOVA_UI_DISABLED;
 unsigned int bg=state&(NOVA_UI_PRESSED|NOVA_UI_SELECTED)?p.selection:state&NOVA_UI_HOVER?p.hover:p.toolbar;
 nova_ui_round(c,x,y,w,h,7,bg);
 if(state&NOVA_UI_FOCUSED){nova_ui_round(c,x,y+h-2,w,2,1,p.accent);}
 return state&NOVA_UI_DISABLED?p.muted:p.ink;
}
/* Shared segmented geometry: painting and hit testing use identical bounds. */
struct nova_ui_rect { int x,y,w,h; };
static inline struct nova_ui_rect nova_ui_segment_rect(int x,int y,int w,int h,unsigned int count,int gap,unsigned int index){
 struct nova_ui_rect result={0,0,0,0};if(!count||count>16U||index>=count||gap<0||h<=0||w<=(int)(count-1U)*gap)return result;
 int bw=(w-(int)(count-1U)*gap)/(int)count;if(bw<=0)return result;result.x=x+(int)index*(bw+gap);result.y=y;result.w=bw;result.h=h;return result;
}
static inline int nova_ui_segment_hit(int x,int y,int w,int h,unsigned int count,int gap,int px,int py){
 for(unsigned int i=0;i<count&&i<16U;i++){struct nova_ui_rect r=nova_ui_segment_rect(x,y,w,h,count,gap,i);if(r.w&&px>=r.x&&px<r.x+r.w&&py>=r.y&&py<r.y+r.h)return (int)i;}return -1;
}
#endif

