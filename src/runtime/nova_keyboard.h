#ifndef NOVA_KEYBOARD_H
#define NOVA_KEYBOARD_H
/* Turkish Q evdev mapping, compared with xkeyboard-config symbols/tr basic.
 * No dead-key composition or general IME is claimed. */
static inline unsigned nova_trq_key(unsigned key,unsigned shift,unsigned ctrl,unsigned altgr,unsigned caps){
 static const unsigned short base[87]={[1]=27,[2]=49,[3]=50,[4]=51,[5]=52,[6]=53,[7]=54,[8]=55,[9]=56,[10]=57,[11]=48,[12]=42,[13]=45,[14]=127,[15]=9,[16]=113,[17]=119,[18]=101,[19]=114,[20]=116,[21]=121,[22]=117,[23]=305,[24]=111,[25]=112,[26]=287,[27]=252,[28]=13,[30]=97,[31]=115,[32]=100,[33]=102,[34]=103,[35]=104,[36]=106,[37]=107,[38]=108,[39]=351,[40]=105,[41]=34,[43]=44,[44]=122,[45]=120,[46]=99,[47]=118,[48]=98,[49]=110,[50]=109,[51]=246,[52]=231,[53]=46,[57]=32,[86]=60};
 static const unsigned short upper[87]={[2]=33,[3]=39,[4]=94,[5]=43,[6]=37,[7]=38,[8]=47,[9]=40,[10]=41,[11]=61,[12]=63,[13]=95,[41]=233,[43]=59,[53]=58,[86]=62};
 static const unsigned short third[87]={[2]=62,[3]=163,[4]=35,[5]=36,[8]=123,[9]=91,[10]=93,[11]=125,[12]=92,[13]=124,[16]=64,[18]=8364,[20]=8378,[27]=126,[40]=39,[41]=60,[43]=96,[86]=124};
 if(key>=87)return 0;
 if(altgr)return third[key];
 unsigned c=base[key];
 if(ctrl){if(c==305)c=105;if(c>=97&&c<=122)return c-96;return 0;}
 unsigned letter=(c>=97&&c<=122)||c==305||c==287||c==252||c==351||c==246||c==231;
 if(letter){if(!!shift!=!!caps){if(c==105)c=304;else if(c==305)c=73;else if(c==287)c=286;else if(c==351)c=350;else c-=32;}}else if(shift&&upper[key])c=upper[key];
 return c;
}
static inline unsigned nova_keyboard_utf8(unsigned c,unsigned char out[4]){
 if(!c||c>0x10ffff||(c>=0xd800&&c<=0xdfff))return 0;
 if(c<128){out[0]=(unsigned char)c;return 1;}
 if(c<2048){out[0]=192|(c>>6);out[1]=128|(c&63);return 2;}
 if(c<65536){out[0]=224|(c>>12);out[1]=128|((c>>6)&63);out[2]=128|(c&63);return 3;}
 out[0]=240|(c>>18);out[1]=128|((c>>12)&63);out[2]=128|((c>>6)&63);out[3]=128|(c&63);return 4;
}
#endif
