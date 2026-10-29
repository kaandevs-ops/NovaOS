#ifndef NOVA_TERMINAL_SIXEL_H
#define NOVA_TERMINAL_SIXEL_H
/* Restricted SIXEL: ESC P q, mandatory "1;1;w;h, RGB percentage palettes,
 * #color, !repeat, $, -, ?..~ and ESC backslash. No files/commands/authority.
 * Palette indexed pixels are lazily cleared by generation (O(1) begin).
 * Accepted frames and paint work are bounded per compositor receive turn. */
#define NOVA_SIXEL_W 800U
#define NOVA_SIXEL_H 600U
#define NOVA_SIXEL_PIXELS (NOVA_SIXEL_W*NOVA_SIXEL_H)
struct nova_sixel {
 unsigned char pixel[NOVA_SIXEL_PIXELS];
 unsigned stamp[NOVA_SIXEL_PIXELS],palette[256];
 unsigned generation,visible,width,height,anchor_x,anchor_y;
 unsigned x,y,color,repeat,declared,bad,command,values[5],field,digits;
 unsigned bytes,paint,turn_paint,turn_frames,rejected;
};
static inline void nova_sixel_turn(struct nova_sixel*s){s->turn_paint=0;s->turn_frames=0;}
static inline void nova_sixel_reject(struct nova_sixel*s){if(!s->bad)s->rejected++;s->bad=1;s->visible=0;}
static inline void nova_sixel_begin(struct nova_sixel*s,unsigned x,unsigned y){
 s->visible=0;s->bad=0;s->width=s->height=s->x=s->y=s->color=s->declared=s->command=s->field=s->digits=s->bytes=s->paint=0;s->repeat=1;s->anchor_x=x;s->anchor_y=y;
 if(++s->turn_frames>2||s->generation==~0U){nova_sixel_reject(s);return;}s->generation++;
 for(unsigned i=0;i<256;i++)s->palette[i]=0;
}
static inline int nova_sixel_command(struct nova_sixel*s){
 unsigned n=s->field+1,*v=s->values;if(!s->digits)return 0;
 if(s->command=='"'){
  if(n!=4||s->declared||v[0]!=1||v[1]!=1||!v[2]||v[2]>NOVA_SIXEL_W||!v[3]||v[3]>NOVA_SIXEL_H)return 0;
  s->width=v[2];s->height=v[3];s->declared=1;
 }else if(s->command=='#'){
  if(v[0]>255||(n!=1&&n!=5))return 0;s->color=v[0];
  if(n==5){if(v[1]!=2||v[2]>100||v[3]>100||v[4]>100)return 0;s->palette[s->color]=((v[2]*255/100)<<16)|((v[3]*255/100)<<8)|(v[4]*255/100);}
 }else if(s->command=='!'){if(n!=1||!v[0]||v[0]>NOVA_SIXEL_W||s->repeat!=1)return 0;s->repeat=v[0];}else return 0;
 s->command=s->field=s->digits=0;return 1;
}
static inline void nova_sixel_byte(struct nova_sixel*s,unsigned char c){
 if(s->bad)return;if(++s->bytes>1048576U){nova_sixel_reject(s);return;}
 if(s->command){
  if(c>='0'&&c<='9'){unsigned*v=&s->values[s->field];if(*v>10000U){nova_sixel_reject(s);return;}*v=*v*10+c-'0';s->digits=1;return;}
  if(c==';'){if(!s->digits||s->field>=4){nova_sixel_reject(s);return;}s->values[++s->field]=0;s->digits=0;return;}
  if(!nova_sixel_command(s)){nova_sixel_reject(s);return;}
 }
 if(c=='"'||c=='#'||c=='!'){s->command=c;s->field=s->digits=0;s->values[0]=0;return;}
 if(c=='$'){if(s->repeat!=1){nova_sixel_reject(s);return;}s->x=0;return;}
 if(c=='-'){if(s->repeat!=1||!s->declared||s->y>=s->height){nova_sixel_reject(s);return;}s->x=0;s->y+=6;return;}
 if(c>=63&&c<=126){
  if(!s->declared||s->y>=s->height||s->x>s->width||s->repeat>s->width-s->x){nova_sixel_reject(s);return;}
  unsigned bits=c-63;
  for(unsigned bit=0;bit<6&&s->y+bit<s->height;bit++)if(bits&(1U<<bit)){
   if(s->repeat>NOVA_SIXEL_PIXELS*2-s->paint||s->repeat>1000000U-s->turn_paint){nova_sixel_reject(s);return;}
   s->paint+=s->repeat;s->turn_paint+=s->repeat;unsigned at=(s->y+bit)*NOVA_SIXEL_W+s->x;
   for(unsigned i=0;i<s->repeat;i++){s->pixel[at+i]=(unsigned char)s->color;s->stamp[at+i]=s->generation;}
  }
  s->x+=s->repeat;s->repeat=1;return;
 }
 if(c=='\r'||c=='\n')return;
 nova_sixel_reject(s);
}
static inline void nova_sixel_end(struct nova_sixel*s){
 if(!s->bad&&s->command&&!nova_sixel_command(s))nova_sixel_reject(s);
 if(!s->bad&&(!s->declared||s->repeat!=1))nova_sixel_reject(s);
 if(!s->bad)s->visible=1;
}
static inline unsigned nova_sixel_color(const struct nova_sixel*s,unsigned x,unsigned y){
 unsigned at=y*NOVA_SIXEL_W+x;return s->stamp[at]==s->generation?s->palette[s->pixel[at]]:0x00ffffffU;
}
#endif
