#ifndef NOVA_READ_MODEL_H
#define NOVA_READ_MODEL_H
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include "nova_home_file.h"
#include "nova_user_path.h"
#include "nova_text_document.h"
#define NOVA_READ_CAP 65536U
struct nova_read_model {
 unsigned char data[NOVA_READ_CAP+1];
 uint32_t line[NOVA_READ_CAP+1];
 size_t length,source_bytes;
 unsigned count,width,top;
};
/* Snapshot content only; no terminal escape, binary, FIFO, links or traversal.
 * Same-UID workspace trust domain, not a cross-principal file portal. */
static inline int nova_read_load(struct nova_read_model*d,const unsigned char*s,size_t n){
 if(n>NOVA_READ_CAP){errno=EFBIG;return -1;}size_t out=0;
 for(size_t i=0;i<n;){
  if(s[i]=='\r'&&i+1<n&&s[i+1]=='\n'){d->data[out++]='\n';i+=2;continue;}
  unsigned k=nova_text_unit(s+i,n-i);if(!k){errno=EILSEQ;return -1;}
  memcpy(d->data+out,s+i,k);out+=k;i+=k;
 }
 d->data[out]=0;d->length=out;d->source_bytes=n;d->top=0;d->count=0;return 0;
}
static inline int nova_read_descriptor(int fd,struct nova_read_model*d){
 struct stat st;unsigned char data[NOVA_READ_CAP+1];size_t used=0;int rc=-1;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink!=1||st.st_uid!=geteuid()){errno=EPERM;goto done;}
 if(st.st_size<0||st.st_size>NOVA_READ_CAP){errno=EFBIG;goto done;}
 while(used<sizeof data){ssize_t n=read(fd,data+used,sizeof data-used);if(n<0&&errno==EINTR)continue;if(n<0)goto done;if(!n)break;used+=(size_t)n;}
 rc=nova_read_load(d,data,used);
 done:{int saved=errno;close(fd);errno=saved;}return rc;
}
static inline int nova_read_file(int root,const char*name,struct nova_read_model*d){
 if(!nova_text_name(name)){errno=EINVAL;return -1;}if(nova_text_root(root))return -1;int fd=openat(root,name,O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);if(fd<0)return -1;return nova_read_descriptor(fd,d);
}
static inline int nova_read_path(int root,const char*home,const char*path,struct nova_read_model*d){
 if(nova_text_root(root))return -1;char name[128];int dir=nova_user_parent(root,home,path,name);if(dir<0)return -1;
 int fd=openat(dir,name,O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);int saved=errno;close(dir);errno=saved;if(fd<0)return -1;return nova_read_descriptor(fd,d);
}
/* One terminal cell per scalar, matching Nova Terminal's current grid.
 * Tabs are shown as >; full Unicode shaping/wcwidth is not claimed. */
static inline int nova_read_layout(struct nova_read_model*d,unsigned width){
 if(width<1||width>240){errno=EINVAL;return -1;}unsigned col=0;d->count=1;d->line[0]=0;d->width=width;
 for(size_t i=0;i<d->length;){unsigned k=nova_text_unit(d->data+i,d->length-i);if(!k)return -1;
  if(d->data[i]=='\n'){i++;d->line[d->count++]=(uint32_t)i;col=0;continue;}
  if(col==width){d->line[d->count++]=(uint32_t)i;col=0;}col++;i+=k;
 }
 return 0;
}
static inline unsigned nova_read_last(const struct nova_read_model*d,unsigned rows){return d->count>rows?d->count-rows:0;}
static inline void nova_read_move(struct nova_read_model*d,int delta,unsigned rows){
 unsigned last=nova_read_last(d,rows);
 if(delta<0){unsigned n=(unsigned)(-(long long)delta);d->top=n>d->top?0:d->top-n;}
 else d->top=(unsigned)delta>last-(d->top>last?last:d->top)?last:d->top+(unsigned)delta;
 if(d->top>last)d->top=last;
}
static inline size_t nova_read_anchor(const struct nova_read_model*d){return d->count?d->line[d->top<d->count?d->top:d->count-1]:0;}
static inline void nova_read_reflow(struct nova_read_model*d,unsigned width,unsigned rows){
 size_t anchor=nova_read_anchor(d);if(nova_read_layout(d,width))return;
 d->top=0;while(d->top+1<d->count&&d->line[d->top+1]<=anchor)d->top++;
 nova_read_move(d,0,rows);
}
static inline int nova_read_find(struct nova_read_model*d,const char*query,size_t start,unsigned rows){
 size_t n=strlen(query);if(!n||n>d->length||start>d->length)return -1;
 for(size_t i=start;i+n<=d->length;i++)if(!memcmp(d->data+i,query,n)){
  unsigned row=0;while(row+1<d->count&&d->line[row+1]<=i)row++;d->top=row;nova_read_move(d,0,rows);return (int)i;
 }return -1;
}
#endif
