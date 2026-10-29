#ifndef NOVA_TERMINAL_GRID_H
#define NOVA_TERMINAL_GRID_H
#include "nova_terminal_protocol.h"
#include "nova_terminal_sixel.h"
#define NOVA_TERM_HISTORY 128U
struct nova_term_grid {
 struct nova_sixel *graphics;
 unsigned int cells[NOVA_TERM_MAX_ROWS][NOVA_TERM_MAX_COLS];
 unsigned int history[NOVA_TERM_HISTORY][NOVA_TERM_MAX_COLS];
 unsigned int rows,cols,x,y,head,history_head,history_count,scroll;
 unsigned int state,params[6],param_count,utf_value,utf_left,utf_min,wrap;
 unsigned long long bytes;
};
static inline void nova_term_blank(unsigned int *row){for(unsigned i=0;i<NOVA_TERM_MAX_COLS;i++)row[i]=' ';}
static inline void nova_term_grid_init(struct nova_term_grid *g,unsigned rows,unsigned cols){
 volatile unsigned char *zero=(volatile unsigned char *)g;for(unsigned long z=0;z<sizeof(*g);z++)zero[z]=0;g->rows=rows>=4&&rows<=NOVA_TERM_MAX_ROWS?rows:24;g->cols=cols>=20&&cols<=NOVA_TERM_MAX_COLS?cols:80;
 for(unsigned y=0;y<NOVA_TERM_MAX_ROWS;y++)nova_term_blank(g->cells[y]);
}
static inline unsigned int *nova_term_live_row(struct nova_term_grid *g,unsigned row){return g->cells[(g->head+row)%g->rows];}
static inline void nova_term_newline(struct nova_term_grid *g){
 g->wrap=0;
 if(++g->y<g->rows)return;
 for(unsigned x=0;x<NOVA_TERM_MAX_COLS;x++)g->history[g->history_head][x]=nova_term_live_row(g,0)[x];
 g->history_head=(g->history_head+1)%NOVA_TERM_HISTORY;if(g->history_count<NOVA_TERM_HISTORY)g->history_count++;
 g->head=(g->head+1)%g->rows;
 nova_term_blank(nova_term_live_row(g,g->rows-1));g->y=g->rows-1;
}
static inline void nova_term_put(struct nova_term_grid *g,unsigned cp){
 if(g->wrap){g->x=0;nova_term_newline(g);}
 nova_term_live_row(g,g->y)[g->x]=cp;if(++g->x>=g->cols){g->x=g->cols-1;g->wrap=1;}
}
static inline void nova_term_csi(struct nova_term_grid *g,unsigned char c){
 unsigned n=g->params[0]?g->params[0]:1;
 if(c=='A')g->y=n>g->y?0:g->y-n;
 else if(c=='B')g->y=n>=g->rows-g->y?g->rows-1:g->y+n;
 else if(c=='C')g->x=n>=g->cols-g->x?g->cols-1:g->x+n;
 else if(c=='D')g->x=n>g->x?0:g->x-n;
 else if(c=='H'||c=='f'){unsigned row=g->params[0]?g->params[0]:1,col=g->params[1]?g->params[1]:1;g->y=row>g->rows?g->rows-1:row-1;g->x=col>g->cols?g->cols-1:col-1;}
 else if(c=='G')g->x=n>g->cols?g->cols-1:n-1;
 else if(c=='J'){unsigned mode=g->params[0];if(mode==2&&g->graphics)g->graphics->visible=0;for(unsigned y=0;y<g->rows;y++)for(unsigned x=0;x<g->cols;x++)if(mode==2||(mode==0&&(y>g->y||(y==g->y&&x>=g->x)))||(mode==1&&(y<g->y||(y==g->y&&x<=g->x))))nova_term_live_row(g,y)[x]=' ';}
 else if(c=='K'){unsigned a=g->params[0]==1?0:g->x,b=g->params[0]==0?g->cols:g->x+1;if(g->params[0]==2){a=0;b=g->cols;}for(unsigned x=a;x<b;x++)nova_term_live_row(g,g->y)[x]=' ';}
 g->wrap=0;
}
static inline void nova_term_feed(struct nova_term_grid *g,const unsigned char *data,unsigned long len){
 g->bytes+=len;
 for(unsigned long i=0;i<len;i++){
  unsigned char c=data[i];
again:
  if(g->state==6){if(c=='q'&&g->graphics){nova_sixel_begin(g->graphics,g->x,g->y);g->state=7;}else g->state=3;continue;}
  if(g->state==7){if(c==27)g->state=8;else if(c==24||c==26){nova_sixel_reject(g->graphics);g->state=0;}else nova_sixel_byte(g->graphics,c);continue;}
  if(g->state==8){if(c=='\\'){nova_sixel_end(g->graphics);g->state=0;}else{nova_sixel_reject(g->graphics);g->state=3;}continue;}
  if(g->state==3){if(c==7)g->state=0;else if(c==27)g->state=4;continue;}
  if(g->state==4){g->state=c=='\\'?0:3;continue;}
  if(g->state==1){g->state=0;if(c=='['){g->state=2;g->param_count=0;for(unsigned j=0;j<6;j++)g->params[j]=0;}else if(c=='P')g->state=g->graphics?6:3;else if(c==']'||c=='^'||c=='_')g->state=3;continue;}
  if(g->state==2){
   if(c>='0'&&c<='9'){unsigned *v=&g->params[g->param_count];*v=*v<10000?*v*10+c-'0':10000;continue;}
   if(c==';'){if(g->param_count<5)g->param_count++;else g->state=5;continue;}
   if(c>=0x40&&c<=0x7e){nova_term_csi(g,c);g->state=0;}else if(c<0x20||c>0x3f)g->state=0;continue;
  }
  if(g->state==5){if(c>=0x40&&c<=0x7e)g->state=0;continue;}
  if(g->utf_left){
   if((c&0xc0)!=0x80){g->utf_left=0;nova_term_put(g,0xfffd);goto again;}
   g->utf_value=(g->utf_value<<6)|(c&63);
   if(!--g->utf_left){unsigned cp=g->utf_value;if(cp<g->utf_min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))cp=0xfffd;nova_term_put(g,cp);}continue;
  }
  if(c==27){g->state=1;continue;}if(c=='\r'){g->x=0;g->wrap=0;continue;}if(c=='\n'){nova_term_newline(g);continue;}
  if(c==8||c==127){if(g->x)g->x--;g->wrap=0;continue;}
  if(c=='\t'){unsigned n=8-g->x%8;while(n--)nova_term_put(g,' ');continue;}
  if(c<32)continue;if(c<128){nova_term_put(g,c);continue;}
  if(c>=0xc2&&c<=0xdf){g->utf_left=1;g->utf_value=c&31;g->utf_min=0x80;}
  else if(c>=0xe0&&c<=0xef){g->utf_left=2;g->utf_value=c&15;g->utf_min=0x800;}
  else if(c>=0xf0&&c<=0xf4){g->utf_left=3;g->utf_value=c&7;g->utf_min=0x10000;}
  else nova_term_put(g,0xfffd);
 }
}
static inline void nova_term_grid_resize(struct nova_term_grid *g,unsigned rows,unsigned cols){
 if(rows<4||rows>NOVA_TERM_MAX_ROWS||cols<20||cols>NOVA_TERM_MAX_COLS)return;
 if(g->head){unsigned temp[NOVA_TERM_MAX_ROWS][NOVA_TERM_MAX_COLS];for(unsigned y=0;y<g->rows;y++)for(unsigned x=0;x<NOVA_TERM_MAX_COLS;x++)temp[y][x]=nova_term_live_row(g,y)[x];for(unsigned y=0;y<g->rows;y++)for(unsigned x=0;x<NOVA_TERM_MAX_COLS;x++)g->cells[y][x]=temp[y][x];g->head=0;}
 if(rows<g->rows&&g->y>=rows){unsigned drop=g->y-rows+1;for(unsigned y=0;y<drop;y++){for(unsigned x=0;x<NOVA_TERM_MAX_COLS;x++)g->history[g->history_head][x]=g->cells[y][x];g->history_head=(g->history_head+1)%NOVA_TERM_HISTORY;if(g->history_count<NOVA_TERM_HISTORY)g->history_count++;}for(unsigned y=0;y<g->rows-drop;y++)for(unsigned x=0;x<NOVA_TERM_MAX_COLS;x++)g->cells[y][x]=g->cells[y+drop][x];g->y-=drop;}
 if(rows>g->rows)for(unsigned y=g->rows;y<rows;y++)nova_term_blank(g->cells[y]);
 if(cols<g->cols)for(unsigned y=0;y<NOVA_TERM_MAX_ROWS;y++)for(unsigned x=cols;x<NOVA_TERM_MAX_COLS;x++)g->cells[y][x]=' ';
 g->rows=rows;g->cols=cols;if(g->x>=cols)g->x=cols-1;if(g->y>=rows)g->y=rows-1;g->wrap=0;
}
static inline const unsigned int *nova_term_view_row(const struct nova_term_grid *g,unsigned row){
 unsigned scroll=g->scroll>g->history_count?g->history_count:g->scroll;
 unsigned index=g->history_count+row-scroll;
 if(index<g->history_count)return g->history[(g->history_head+NOVA_TERM_HISTORY-g->history_count+index)%NOVA_TERM_HISTORY];
 return g->cells[(g->head+index-g->history_count)%g->rows];
}
static inline unsigned nova_term_key_ascii(unsigned key,unsigned shift,unsigned ctrl){
 static const unsigned char plain[58]={ [1]=27,[2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',[12]='-',[13]='=',[14]=127,[15]='\t',[16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\r',[30]='a',[31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'',[41]='`',[43]='\\',[44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[57]=' '};
 static const unsigned char shifted[58]={ [2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',[12]='_',[13]='+',[26]='{',[27]='}',[39]=':',[40]='"',[41]='~',[43]='|',[51]='<',[52]='>',[53]='?'};
 if(key>=58)return 0;unsigned c=plain[key];if(ctrl&&c>='a'&&c<='z')return c-'a'+1;if(shift){if(c>='a'&&c<='z')c-=32;else if(shifted[key])c=shifted[key];}return c;
}
#endif
