/* Unit test of modifier metadata translation, not a guest/X server test. */
#define nova_x11_key recorded_key
#include "../src/compat/nova_browser_bridge.c"
#undef nova_x11_key
#include <assert.h>
#include <stdio.h>
static unsigned codes[16],values[16],count;
int recorded_key(struct nova_x11*x,unsigned code,int value){(void)x;assert(count<16);codes[count]=code;values[count++]=value;return 0;}
int main(void){struct bridge b={0};
 assert(!sync_modifiers(&b,NOVA_INPUT_MOD_CTRL));assert(count==1&&codes[0]==29&&values[0]==1);
 assert(!sync_modifiers(&b,NOVA_INPUT_MOD_CTRL));assert(count==1);
 assert(!sync_modifiers(&b,NOVA_INPUT_MOD_SHIFT));assert(count==3&&codes[1]==29&&!values[1]&&codes[2]==42&&values[2]);
 assert(!sync_modifiers(&b,0));assert(count==4&&codes[3]==42&&!values[3]);
 assert(sync_modifiers(&b,128)==-1&&count==4&&b.modifiers==0);
 puts("PASS modifier metadata: Ctrl, Shift, transitions, duplicate suppression, unknown-bit rejection");return 0;}
