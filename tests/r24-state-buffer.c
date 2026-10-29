#include "nova_runtime.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static char output[100000];static unsigned long length,calls;static int short_mode,interrupt_once,fail_mode;
static long test_write(int fd,const void *p,unsigned long n){assert(fd==7);calls++;if(interrupt_once){interrupt_once=0;return -NOVA_EINTR;}if(fail_mode)return -5;if(short_mode&&n>7)n=7;assert(length+n<=sizeof(output));memcpy(output+length,p,n);length+=n;return (long)n;}
#define nova_write test_write
#include "nova_state_buffer.h"
#undef nova_write
static void reset(void){memset(&g_nova_state,0,sizeof(g_nova_state));length=calls=0;short_mode=interrupt_once=fail_mode=0;}
int main(void){
 reset();assert(nova_state_begin(7));assert(!nova_state_begin(8));
 char expected[20000];unsigned int used=0;
 for(unsigned int i=0;i<1000;i++){nova_state_puts(7,"counter=");nova_state_put_u64(7,i);nova_state_putln(7,"");used+=(unsigned int)sprintf(expected+used,"counter=%u\n",i);}
 assert(!calls);assert(nova_state_end());assert(calls==1&&length==used&&!memcmp(expected,output,used));assert(!g_nova_state.active);
 reset();assert(nova_state_begin(7));for(unsigned int i=0;i<70000;i++)nova_state_puts(7,"x");assert(nova_state_end());assert(length==70000&&calls==3);for(unsigned int i=0;i<length;i++)assert(output[i]=='x');
 reset();short_mode=1;interrupt_once=1;assert(nova_state_begin(7));nova_state_putln(7,"short write and EINTR");assert(nova_state_end());assert(length==22&&!memcmp(output,"short write and EINTR\n",22));
 reset();assert(nova_state_begin(7));nova_state_puts(7,"error");fail_mode=1;assert(!nova_state_end());assert(!g_nova_state.active&&calls==1);fail_mode=0;assert(nova_state_begin(7));nova_state_putln(7,"recovered");assert(nova_state_end());assert(length==10&&!memcmp(output,"recovered\n",10));
 puts("R24_STATE_BUFFER_BYTE_EQUIVALENCE_1000_FIELDS_1_WRITE_BOUNDS_SHORT_EINTR_ERROR_RECOVERY=PASS");return 0;
}
