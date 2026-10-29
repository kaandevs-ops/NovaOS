#ifndef NOVA_TIME_VIEW_H
#define NOVA_TIME_VIEW_H
#include "nova_calendar.h"
#include "nova_ui_font.h"
static inline void nova_time_number(unsigned int v,char*out){char rev[16];unsigned int n=0,k=0;do{rev[n++]=(char)('0'+v%10);v/=10;}while(v);while(n)out[k++]=rev[--n];out[k]=0;}
static inline void nova_time_view(struct nova_ui_canvas c,struct nova_ui_palette p,int x,int y,int width,int height,int clock,int year,unsigned int month,const struct nova_date*now){
 if(width<280||height<310)return;
 nova_ui_round(c,x,y,width,height,14,p.paper);
 const char*title=clock?"Clock":"Calendar";
 nova_ui_text(c,x+20,y+34,NOVA_UI_TITLE,1,p.ink,title,x+16,y+8,width-64,36);
 nova_ui_button(c,x+width-42,y+10,30,30,p,0);nova_ui_text(c,x+width-32,y+31,NOVA_UI_BODY,1,p.ink,"x",x+width-42,y+10,30,30);
 if(!now){nova_ui_text(c,x+20,y+90,NOVA_UI_BODY,1,p.muted,"System time unavailable",x+16,y+60,width-32,40);return;}
 if(clock){char time[]={ (char)('0'+now->hour/10),(char)('0'+now->hour%10),':',(char)('0'+now->minute/10),(char)('0'+now->minute%10),':',(char)('0'+now->second/10),(char)('0'+now->second%10),0};
  nova_ui_text(c,x+24,y+138,NOVA_UI_TITLE,2,p.ink,time,x+16,y+78,width-32,70);
  nova_ui_text(c,x+24,y+188,NOVA_UI_BODY,1,p.muted,"UTC / guest system clock",x+16,y+160,width-32,40);
  nova_ui_text(c,x+24,y+232,NOVA_UI_CAPTION,1,p.muted,"Network time sync is not configured",x+16,y+200,width-32,40);return;}
 const char*names[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
 if(!nova_calendar_days(year,month))return;
 char label[40],digits[16];nova_time_number((unsigned int)year,digits);unsigned int n=0,k=0;while(names[month-1][n]){label[n]=names[month-1][n];n++;}label[n++]=' ';while(digits[k])label[n++]=digits[k++];label[n]=0;
 nova_ui_text(c,x+20,y+80,NOVA_UI_HEADING,1,p.ink,label,x+16,y+52,width-140,36);
 const char*controls[]={"<","Today",">"};int offsets[]={width-132,width-96,width-40},sizes[]={30,50,30};for(int i=0;i<3;i++){nova_ui_button(c,x+offsets[i],y+54,sizes[i],30,p,0);nova_ui_text(c,x+offsets[i]+7,y+75,NOVA_UI_CAPTION,1,p.ink,controls[i],x+offsets[i],y+54,sizes[i],30);}
 const char*week[]={"Mo","Tu","We","Th","Fr","Sa","Su"};int cell=(width-32)/7,row=(height-156)/6;if(row<20)row=20;
 for(int i=0;i<7;i++)nova_ui_text(c,x+16+i*cell+8,y+120,NOVA_UI_CAPTION,1,p.muted,week[i],x+16+i*cell,y+98,cell,28);
 unsigned int first=nova_calendar_first(year,month),days=nova_calendar_days(year,month);
 for(unsigned int d=1;d<=days;d++){unsigned int pos=first+d-1;int dx=x+16+(int)(pos%7)*cell,dy=y+132+(int)(pos/7)*row;int current=year==now->year&&month==now->month&&d==now->day;
 if(current)nova_ui_round(c,dx+2,dy,cell-4,row-2,6,p.selection);
 nova_time_number(d,digits);nova_ui_text(c,dx+8,dy+20,NOVA_UI_BODY,1,current?p.accent:p.ink,digits,dx,dy,cell,row);}
}
#endif
