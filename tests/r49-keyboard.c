#include <assert.h>
#include "nova_keyboard.h"
int main(void){
assert(nova_trq_key(40,0,0,0,0)==105);
assert(nova_trq_key(40,1,0,0,0)==304);
assert(nova_trq_key(23,0,0,0,0)==305);
assert(nova_trq_key(23,1,0,0,0)==73);
assert(nova_trq_key(40,0,0,0,1)==304);
assert(nova_trq_key(40,1,0,0,1)==105);
assert(nova_trq_key(8,1,0,0,0)==47);
assert(nova_trq_key(13,0,1,1,0)==124);
assert(nova_trq_key(46,0,1,0,0)==3);
assert(nova_trq_key(3,1,0,0,0)==39);
assert(nova_trq_key(41,0,0,0,0)==34);
unsigned char b[4];assert(nova_keyboard_utf8(305,b)==2&&b[0]==0xc4&&b[1]==0xb1);
for(unsigned k=0;k<1024;k++)for(unsigned m=0;m<32;m++){unsigned c=nova_trq_key(k,m&1,m&2,m&4,m&8);assert(c<0x110000);if(c)assert(nova_keyboard_utf8(c,b)>0);}
return 0;}
