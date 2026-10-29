#ifndef NOVA_TERMINAL_PROTOCOL_H
#define NOVA_TERMINAL_PROTOCOL_H
/* Private inherited SOCK_SEQPACKET channel, not a public IPC endpoint.
 * Only validated Desktop basenames / trash tokens in file operations; no
 * arbitrary paths, commands, identity claims or executable choices. */
#define NOVA_TERM_MAGIC 0x4e54524dU
#define NOVA_TERM_VERSION 1U
#define NOVA_TERM_BYTES 2048U
#define NOVA_TERM_FD 3
#define NOVA_TERM_UID 12004U
#define NOVA_TERM_PERSISTENT 0x10000U
#define NOVA_TERM_MAX_ROWS 48U
#define NOVA_TERM_MAX_COLS 120U
#ifndef NOVA_TERM_HELPER
#define NOVA_TERM_HELPER "/System/Core/nova-terminal-session"
#endif
#ifndef NOVA_TERM_BUNDLE
#define NOVA_TERM_BUNDLE "/System/Terminal"
#endif
enum { NOVA_TERM_INPUT=1,NOVA_TERM_RESIZE=2,NOVA_TERM_CLOSE=3,
       NOVA_TERM_DESKTOP=4, NOVA_TERM_DESKTOP_FILE=5, NOVA_TERM_DESKTOP_STATE=20,
       NOVA_TERM_DESKTOP_FILE_STATE=21, NOVA_TERM_DESKTOP_TRASH_STATE=22,
       NOVA_TERM_READY=16,NOVA_TERM_DATA=17,NOVA_TERM_EXIT=18,NOVA_TERM_ERROR=19 };
#define NOVA_DESKTOP_LIMIT 24U
struct nova_desktop_entry { unsigned int kind; char name[64]; };
struct nova_desktop_snapshot { unsigned int status,count,clipped,reserved; struct nova_desktop_entry entries[NOVA_DESKTOP_LIMIT]; };
#include "nova_desktop_files_protocol.h"
struct nova_term_packet {
 unsigned int magic,version,op,length,rows,cols,code,reserved;
 unsigned char data[NOVA_TERM_BYTES];
};
#define NOVA_TERM_HEADER 32U
_Static_assert(sizeof(struct nova_term_packet)==NOVA_TERM_HEADER+NOVA_TERM_BYTES,"terminal packet ABI");
static inline int nova_term_valid(const struct nova_term_packet *p,unsigned long bytes,int request){
 if(!p||bytes<NOVA_TERM_HEADER||p->magic!=NOVA_TERM_MAGIC||p->version!=NOVA_TERM_VERSION||p->reserved||p->length>NOVA_TERM_BYTES||bytes!=NOVA_TERM_HEADER+p->length)return 0;
 if(request){
  if(p->code)return 0;
  if(p->op==NOVA_TERM_INPUT)return p->length>0&&!p->rows&&!p->cols;
  if(p->op==NOVA_TERM_DESKTOP_FILE){
   if(p->length!=sizeof(struct nova_desktop_file_request)||p->rows||p->cols)return 0;
   const struct nova_desktop_file_request*r=(const void*)p->data;
   return nova_desktop_file_request_valid(r);
  }
  if(p->op==NOVA_TERM_DESKTOP)return !p->length&&p->rows>=1&&p->rows<=3&&!p->cols;
  if(p->op==NOVA_TERM_RESIZE)return !p->length&&p->rows>=4&&p->rows<=NOVA_TERM_MAX_ROWS&&p->cols>=20&&p->cols<=NOVA_TERM_MAX_COLS;
  return p->op==NOVA_TERM_CLOSE&&!p->length&&!p->rows&&!p->cols;
 }
 if(p->op==NOVA_TERM_DESKTOP_FILE_STATE)return p->length==sizeof(struct nova_desktop_file_reply)&&!p->rows&&!p->cols&&!p->code;
 if(p->op==NOVA_TERM_DESKTOP_TRASH_STATE)return p->length==sizeof(struct nova_desktop_trash_snapshot)&&!p->rows&&!p->cols&&!p->code;
 if(p->op==NOVA_TERM_DESKTOP_STATE)return p->length==sizeof(struct nova_desktop_snapshot)&&!p->rows&&!p->cols&&!p->code;
 if(p->op==NOVA_TERM_DATA)return p->length>0&&!p->rows&&!p->cols&&!p->code;
 if(p->op==NOVA_TERM_READY)return !p->length&&(p->code==NOVA_TERM_UID||p->code==(NOVA_TERM_UID|NOVA_TERM_PERSISTENT))&&p->rows>=4&&p->rows<=NOVA_TERM_MAX_ROWS&&p->cols>=20&&p->cols<=NOVA_TERM_MAX_COLS;
 return (p->op==NOVA_TERM_EXIT||p->op==NOVA_TERM_ERROR)&&!p->rows&&!p->cols&&!p->length;
}
#endif
