#ifndef NOVA_CALENDAR_H
#define NOVA_CALENDAR_H
/* Allocation-free Gregorian UTC model. No filesystem/network authority. */
struct nova_date {int year;unsigned int month,day,hour,minute,second,weekday;};
static inline unsigned int nova_calendar_days(int y,unsigned int m){
 static const unsigned int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
 if(y<1970||y>9999||m<1||m>12)return 0;
 return days[m-1]+(m==2&&y%4==0&&(y%100!=0||y%400==0));
}
static inline int nova_calendar_utc(long long seconds,struct nova_date*out){
 if(!out||seconds<0||seconds>253402300799LL)return -1;
 long long days=seconds/86400,remaining=days;int y=1970;unsigned int m=1;
 while(remaining>=(long long)(365+(y%4==0&&(y%100!=0||y%400==0)))){remaining-=365+(y%4==0&&(y%100!=0||y%400==0));y++;}
 while(remaining>=nova_calendar_days(y,m)){remaining-=nova_calendar_days(y,m);m++;}
 struct nova_date d={y,m,(unsigned int)remaining+1,(unsigned int)(seconds/3600%24),(unsigned int)(seconds/60%60),(unsigned int)(seconds%60),(unsigned int)((days+3)%7)};*out=d;return 0;
}
static inline int nova_calendar_step(int*y,unsigned int*m,int direction){
 if(!y||!m||!nova_calendar_days(*y,*m)||(direction!=1&&direction!=-1))return -1;
 int yy=*y;unsigned int mm=*m;
 if(direction>0){if(mm==12){yy++;mm=1;}else mm++;}else{if(mm==1){yy--;mm=12;}else mm--;}
 if(!nova_calendar_days(yy,mm))return -1;*y=yy;*m=mm;return 0;
}
static inline unsigned int nova_calendar_first(int y,unsigned int m){
 if(!nova_calendar_days(y,m))return 7;
 unsigned long long days=0;for(int i=1970;i<y;i++)days+=365+(i%4==0&&(i%100!=0||i%400==0));
 for(unsigned int i=1;i<m;i++)days+=nova_calendar_days(y,i);
 return (unsigned int)((days+3)%7); /* Monday=0 */
}
#endif
