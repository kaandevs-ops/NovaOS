#ifndef NOVA_TEXT_DOCUMENT_H
#define NOVA_TEXT_DOCUMENT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define NOVA_TEXT_CAP 4096U
struct nova_text_document { unsigned char data[NOVA_TEXT_CAP]; size_t length,cursor; };
/* Strict UTF-8; terminal controls are never accepted as document text. */
static inline unsigned nova_text_unit(const unsigned char *s,size_t n){
 if(!n)return 0;unsigned c=s[0],count;uint32_t v,min;
 if(c<128)return c==10||c==9||(c>=32&&c!=127)?1:0;
 if(c>=0xc2&&c<=0xdf){count=2;v=c&31;min=0x80;}
 else if(c>=0xe0&&c<=0xef){count=3;v=c&15;min=0x800;}
 else if(c>=0xf0&&c<=0xf4){count=4;v=c&7;min=0x10000;}else return 0;
 if(n<count)return 0;for(unsigned i=1;i<count;i++){if((s[i]&0xc0)!=0x80)return 0;v=(v<<6)|(s[i]&63);}
 return v<min||v>0x10ffff||(v>=0xd800&&v<=0xdfff)||(v>=0x80&&v<=0x9f)?0:count;
}
static inline int nova_text_load(struct nova_text_document*d,const unsigned char*s,size_t n){
 if(n>NOVA_TEXT_CAP)return -1;for(size_t i=0;i<n;){unsigned k=nova_text_unit(s+i,n-i);if(!k)return -1;i+=k;}
 if(n)memcpy(d->data,s,n);d->length=n;d->cursor=n;return 0;
}
static inline int nova_text_insert(struct nova_text_document*d,const unsigned char*s,size_t n){
 if(!n||n>NOVA_TEXT_CAP-d->length)return -1;
 for(size_t i=0;i<n;){unsigned k=nova_text_unit(s+i,n-i);if(!k)return -1;i+=k;}
 memmove(d->data+d->cursor+n,d->data+d->cursor,d->length-d->cursor);memcpy(d->data+d->cursor,s,n);d->cursor+=n;d->length+=n;return 0;
}
static inline void nova_text_left(struct nova_text_document*d){if(d->cursor){d->cursor--;while(d->cursor&&(d->data[d->cursor]&0xc0)==0x80)d->cursor--;}}
static inline void nova_text_right(struct nova_text_document*d){if(d->cursor<d->length)d->cursor+=nova_text_unit(d->data+d->cursor,d->length-d->cursor);}
static inline void nova_text_backspace(struct nova_text_document*d){size_t end=d->cursor;nova_text_left(d);memmove(d->data+d->cursor,d->data+end,d->length-end);d->length-=end-d->cursor;}
static inline void nova_text_delete(struct nova_text_document*d){size_t start=d->cursor;nova_text_right(d);size_t end=d->cursor;d->cursor=start;memmove(d->data+start,d->data+end,d->length-end);d->length-=end-start;}
static inline void nova_text_home(struct nova_text_document*d){while(d->cursor&&d->data[d->cursor-1]!='\n')nova_text_left(d);}
static inline void nova_text_end(struct nova_text_document*d){while(d->cursor<d->length&&d->data[d->cursor]!='\n')nova_text_right(d);}
static inline void nova_text_vertical(struct nova_text_document*d,int down){
 size_t original=d->cursor;unsigned column=0;nova_text_home(d);size_t start=d->cursor;
 while(d->cursor<original){nova_text_right(d);column++;}d->cursor=start;
 if(down){nova_text_end(d);if(d->cursor==d->length){d->cursor=original;return;}nova_text_right(d);}
 else{if(!start){d->cursor=original;return;}nova_text_left(d);nova_text_home(d);}
 while(column--&&d->cursor<d->length&&d->data[d->cursor]!='\n')nova_text_right(d);
}
#endif
