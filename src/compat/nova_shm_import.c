#define _GNU_SOURCE
#include "nova_shm_import.h"
#include "nova_surface_protocol.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
_Static_assert(sizeof(off_t)>=8,"64-bit pread required");
static uint32_t flatten(uint32_t pixel,uint32_t bg){unsigned a=pixel>>24;uint32_t result=0;for(unsigned shift=0;shift<24;shift+=8){unsigned c=(pixel>>shift)&255;if(c>a)c=a;c+=(((bg>>shift)&255)*(255-a)+127)/255;result|=c<<shift;}return result;}
int nova_shm_import(int fd,uid_t peer,const struct nova_shm_spec*s,uint32_t bg,void*out,size_t cap){
 if(fd<0||!s||!out||!s->width||!s->height||s->width>1920||s->height>1080||s->format>1||s->stride%4||s->stride<s->width*4||s->stride>16384)return -EINVAL;
 uint64_t bytes=(uint64_t)s->width*s->height*4,span=(uint64_t)(s->height-1)*s->stride+s->width*4;
 if(bytes>NOVA_SURFACE_MAX_BUFFER_BYTES||bytes>cap||s->pool_bytes>NOVA_SURFACE_MAX_BUFFER_BYTES||s->offset>s->pool_bytes||span>s->pool_bytes-s->offset)return -EOVERFLOW;
 struct stat before;if(fstat(fd,&before))return -errno;
 if(!S_ISREG(before.st_mode)||before.st_uid!=peer||before.st_size<0||(uint64_t)before.st_size<s->pool_bytes||fcntl(fd,F_GET_SEALS)<0)return -EACCES;
 unsigned char row[1920*4];
 for(uint32_t y=0;y<s->height;y++){
  size_t got=0,need=(size_t)s->width*4;while(got<need){ssize_t n=pread(fd,row+got,need-got,(off_t)(s->offset+(uint64_t)y*s->stride+got));if(n<0&&errno==EINTR)continue;if(n<0)return -errno;if(!n)return -EIO;got+=(size_t)n;}
  for(uint32_t x=0;x<s->width;x++){uint32_t pixel;memcpy(&pixel,row+x*4,4);pixel=s->format==NOVA_SHM_XRGB8888?pixel&0xffffffU:flatten(pixel,bg);memcpy((unsigned char*)out+((size_t)y*s->width+x)*4,&pixel,4);}
 }
 struct stat after;if(fstat(fd,&after)||after.st_size!=before.st_size)return -EAGAIN;
 return 0;
}
