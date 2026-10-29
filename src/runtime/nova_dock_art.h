#ifndef NOVA_DOCK_ART_H
#define NOVA_DOCK_ART_H
/* Original NovaOS dock art. Build-independent 3x raster cache, no allocation,
 * files, network, floating point or per-frame rasterisation. 5 identities only. */
#define NOVA_DOCK_SYSTEM_ITEMS 4U
#define NOVA_DOCK_FRAME_PAD 26U
#define NOVA_DOCK_ICON_SIZE 48
#define NOVA_DOCK_SUPER 144
struct nova_dock_cache { unsigned ready; unsigned pixels[48*48]; };
static struct nova_dock_cache nova_dock_cache[10];
static unsigned nova_dock_scratch[144*144];
static unsigned nova_dock_cache_builds;
static int nova_dock_inside(int x,int y){
 int r=31,dx=x<r?r-x-1:x>=144-r?x-144+r:0;
 int dy=y<r?r-y-1:y>=144-r?y-144+r:0;
 return !dx||!dy||dx*dx+dy*dy<r*r;
}
static void nova_dock_prepare(unsigned slot){
 struct nova_dock_cache*k=&nova_dock_cache[slot];if(k->ready)return;
 unsigned top[]={0x536b9b,0x667487,0x26394a,0x3f9dd2,0x248b9b,0x7f70ba,0x438eac,0x4979af,0x6476af,0xc79551};
 unsigned bottom[]={0x273b65,0x303d50,0x111f30,0x205698,0x234a7d,0x453672,0x245270,0x263d78,0x403568,0x946037};
 struct nova_ui_canvas c={(unsigned char*)nova_dock_scratch,144,144,144*4};
 for(unsigned y=0;y<144;y++)for(unsigned x=0;x<144;x++){
  unsigned t=top[slot],b=bottom[slot],v=0;
  for(unsigned shift=0;shift<=16;shift+=8)v|=((((t>>shift)&255)*(143-y)+((b>>shift)&255)*y)/143)<<shift;
  nova_dock_scratch[y*144+x]=v;
 }
 nova_ui_round(c,9,7,126,3,1,0x95b4d3);
 if(slot>=6){
 if(slot==6){nova_ui_round(c,35,37,74,71,16,0xdbeef3);nova_ui_round(c,35,30,74,30,15,0x91d0dd);nova_ui_line(c,39,71,105,71,0x4e91a8);nova_ui_line(c,39,92,105,92,0x4e91a8);}
 else if(slot==9){for(int j=0;j<2;j++)for(int i=0;i<2;i++)nova_ui_round(c,29+i*47,29+j*47,39,39,9,(i+j)%2?0xf3ddae:0xfff0d3);}
 else{nova_ui_round(c,35,35,76,76,38,0xdce8f8);nova_ui_round(c,58,28,58,58,29,slot==7?0x3c65a0:0x5a629d);nova_ui_round(c,100,25,14,14,7,0xf5e5a5);}
 }else if(slot==5){nova_ui_round(c,31,48,82,68,12,0xe7e4f7);nova_ui_round(c,49,26,46,49,22,0xe7e4f7);nova_ui_round(c,57,34,30,40,15,0x695a9e);nova_ui_round(c,31,53,82,63,12,0xe7e4f7);nova_ui_line(c,72,68,72,97,0x5d4b96);nova_ui_line(c,58,83,86,83,0x5d4b96);
 }else if(slot==0){
  const unsigned colors[]={0xecb66e,0x78d4c1,0xabc5f4,0xe3a2a9};
  for(int j=0;j<2;j++)for(int i=0;i<2;i++){int x=31+i*46,y=30+j*46;nova_ui_round(c,x,y+3,36,36,10,0x172b4d);nova_ui_round(c,x,y,36,36,10,colors[j*2+i]);nova_ui_round(c,x+5,y+4,26,2,1,0xf4f4ed);}
 }else if(slot==1){
  const int pos[][2]={{64,25},{64,101},{25,64},{101,64},{36,36},{92,36},{36,92},{92,92}};
  for(unsigned i=0;i<8;i++)nova_ui_round(c,pos[i][0],pos[i][1],17,18,5,0xd9e4f0);
  nova_ui_round(c,35,35,75,75,37,0xe4edf7);nova_ui_round(c,48,48,49,49,24,0x839ab4);nova_ui_round(c,56,56,33,33,16,0x354a65);nova_ui_round(c,61,59,22,3,1,0x9fb8d4);
 }else if(slot==2){
  nova_ui_line(c,35,50,56,71,0xc5f0de);nova_ui_line(c,56,71,35,92,0xc5f0de);
  nova_ui_line(c,36,50,57,71,0xc5f0de);nova_ui_line(c,57,71,36,92,0xc5f0de);
  nova_ui_round(c,68,92,39,6,3,0xc5f0de);
 }else if(slot==3){
  nova_ui_round(c,26,34,45,28,7,0xbeddf6);nova_ui_round(c,21,46,102,64,13,0xe9f5ff);nova_ui_round(c,30,52,83,26,5,0xb6d6f2);nova_ui_round(c,20,65,104,49,12,0x85bceb);nova_ui_round(c,29,68,86,3,1,0xdcefff);
 }else{
  nova_ui_round(c,29,29,86,86,43,0xa3e6ea);nova_ui_round(c,34,34,76,76,38,0x306e9d);
  for(int y=36;y<109;y++)for(int x=36;x<109;x++){
   int dx=x-72,dy=y-72,ellipse=dx*dx*4+dy*dy;
   if(ellipse>1180&&ellipse<1420)nova_ui_pixel(c,x,y,0xbbe5f2,230);
  }
  nova_ui_round(c,32,70,80,3,1,0xbbe5f2);nova_ui_round(c,40,49,64,3,1,0x9ccddd);nova_ui_round(c,40,92,64,3,1,0x9ccddd);
  nova_ui_round(c,93,29,16,16,8,0xf2ca88);
 }
 for(unsigned y=0;y<48;y++)for(unsigned x=0;x<48;x++){
  unsigned red=0,green=0,blue=0,a=0;
  for(unsigned sy=0;sy<3;sy++)for(unsigned sx=0;sx<3;sx++)if(nova_dock_inside((int)x*3+sx,(int)y*3+sy)){
   unsigned color=nova_dock_scratch[(y*3+sy)*144+x*3+sx];red+=color>>16&255;green+=color>>8&255;blue+=color&255;a++;
  }
  k->pixels[y*48+x]=((a*255/9)<<24)|((red/9)<<16)|((green/9)<<8)|(blue/9);
 }
 k->ready=1;nova_dock_cache_builds++;
}
static void nova_ui_dock_icon(struct nova_ui_canvas c,int x,int y,unsigned identity){
 unsigned slot=identity==0?0:identity==4?1:identity==6?2:identity==3?3:identity>=7&&identity<=11?identity-2:4;
 nova_dock_prepare(slot);
 for(int j=0;j<48;j++)for(int i=0;i<48;i++){
  unsigned v=nova_dock_cache[slot].pixels[j*48+i],a=v>>24;if(!a)continue;
  int px=x+i,py=y+j;if(px<0||py<0||(unsigned)px>=c.width||(unsigned)py>=c.height)continue;
  unsigned*d=(unsigned*)(c.pixels+(unsigned long)py*c.pitch)+px,b=*d,inv=255-a;
  unsigned red=(v>>16&255)+((b>>16&255)*inv+127)/255,green=(v>>8&255)+((b>>8&255)*inv+127)/255,blue=(v&255)+((b&255)*inv+127)/255;
  *d=((red>255?255:red)<<16)|((green>255?255:green)<<8)|(blue>255?255:blue);
 }
}
/* Explicit state, shared with tests: hidden/minimized != stopped. */
static unsigned nova_dock_state(unsigned live,unsigned visible,unsigned focused,unsigned input_blocked){return !live?0:focused&&visible&&!input_blocked?3:visible?2:1;}
#endif
