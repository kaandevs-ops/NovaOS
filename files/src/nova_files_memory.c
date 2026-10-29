/* Compilers may lower C structure assignments to memcpy even with -fno-builtin. */
#include "nova_runtime.h"
void *memcpy(void *dst,const void *src,unsigned long n) {
 unsigned char *d=dst;const unsigned char *s=src;
 for(unsigned long i=0;i<n;i++)d[i]=s[i];return dst;
}
