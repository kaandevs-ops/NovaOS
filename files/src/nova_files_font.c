#include "nova_files_font.h"
#include "../../src/runtime/nova_ui_font.h"
/* Compatibility entry point: legacy top-origin callers; new native UI uses baseline API. */
void nova_files_font_text(unsigned char*b,unsigned int w,unsigned int h,unsigned int p,int x,int y,
 unsigned int style,unsigned int color,const char*text,int cx,int cy,unsigned int cw,unsigned int ch){
 if(style>1U)return;struct nova_ui_canvas canvas={b,w,h,p};
 nova_ui_text(canvas,x,y+nova_ui_face_for(style,1)->ascent,style,1,color,text,cx,cy,cw,ch);
}
void nova_files_font_ellipsis(const char*src,char*out,unsigned int cap,unsigned int style,unsigned int width){
 if(style>1U){if(out&&cap)out[0]=0;return;}nova_ui_ellipsis(src,out,cap,style,width);
}
