#ifndef NOVA_DESKTOP_FILES_PROTOCOL_H
#define NOVA_DESKTOP_FILES_PROTOCOL_H
/* Additive v1 private-channel operations. Only Desktop basenames or opaque
 * trash tokens; never paths, command strings, program choices or new FDs. */
#define NOVA_DESKTOP_TEXT_BYTES 1800U
#define NOVA_DESKTOP_TEXT_LIMIT (256U*1024U)
#define NOVA_DESKTOP_TRASH_LIMIT 12U
enum { NOVA_DESKTOP_PREVIEW=1, NOVA_DESKTOP_TRASH_MOVE=2,
       NOVA_DESKTOP_TRASH_RESTORE=3, NOVA_DESKTOP_TRASH_LIST=4 };
struct nova_desktop_file_request { unsigned action,offset; char name[96]; };
struct nova_desktop_file_reply { unsigned status,action,offset,total,length,reserved; char name[64]; unsigned char text[NOVA_DESKTOP_TEXT_BYTES]; };
struct nova_desktop_trash_entry { unsigned kind; char name[64],token[96]; };
struct nova_desktop_trash_snapshot { unsigned status,count,clipped,reserved,start,total; struct nova_desktop_trash_entry entries[NOVA_DESKTOP_TRASH_LIMIT]; };
static inline unsigned nova_desktop_basename_length(const char*n,unsigned cap){
 if(!n||!cap||!n[0]||n[0]=='.')return 0;
 for(unsigned i=0;i<cap;i++){unsigned c=(unsigned char)n[i];if(!c)return i;if(c<32||c==127||c=='/'||c=='\\')return 0;}return 0;
}
static inline int nova_desktop_token_valid(const char*n){
 unsigned len=nova_desktop_basename_length(n,96);if(len<20||n[0]!='t'||n[1]!='-'||n[18]!='-')return 0;
 for(unsigned i=2;i<18;i++)if(!((n[i]>='0'&&n[i]<='9')||(n[i]>='a'&&n[i]<='f')))return 0;
 return nova_desktop_basename_length(n+19,64)>0;
}
static inline int nova_desktop_file_request_valid(const struct nova_desktop_file_request*r){
 if(r->action==NOVA_DESKTOP_TRASH_LIST)return r->offset<=256&&!r->name[0];
 if(r->action==NOVA_DESKTOP_TRASH_RESTORE)return !r->offset&&nova_desktop_token_valid(r->name);
 return (r->action==NOVA_DESKTOP_PREVIEW||r->action==NOVA_DESKTOP_TRASH_MOVE)&&
        nova_desktop_basename_length(r->name,64)&&
        (r->action==NOVA_DESKTOP_PREVIEW?r->offset<=NOVA_DESKTOP_TEXT_LIMIT:!r->offset);
}
/* Validate UTF-8 scalar values and printable text. Return a safe prefix when a
 * page ends inside a code point. No terminal escape/control interpretation. */
static inline int nova_desktop_text_prefix(const unsigned char*s,unsigned n,int final){
 unsigned i=0;
 while(i<n){unsigned c=s[i],bytes=1,cp=c;
  if(c<32&&c!=9&&c!=10&&c!=13)return -1;if(c==127)return -1;
  if(c>=128){if(c>=0xc2&&c<=0xdf){bytes=2;cp=c&31;}else if(c>=0xe0&&c<=0xef){bytes=3;cp=c&15;}else if(c>=0xf0&&c<=0xf4){bytes=4;cp=c&7;}else return -1;
   if(bytes>n-i)return final?-1:(int)i;
   for(unsigned j=1;j<bytes;j++){unsigned q=s[i+j];if((q&0xc0)!=0x80)return -1;cp=(cp<<6)|(q&63);}
   if((bytes==3&&cp<0x800)||(bytes==4&&cp<0x10000)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return -1;
  }i+=bytes;
 }return (int)i;
}
_Static_assert(sizeof(struct nova_desktop_file_reply)<=2048,"desktop preview bound");
_Static_assert(sizeof(struct nova_desktop_trash_snapshot)<=2048,"trash snapshot bound");
#endif
