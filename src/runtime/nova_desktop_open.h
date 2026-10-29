#ifndef NOVA_DESKTOP_OPEN_H
#define NOVA_DESKTOP_OPEN_H
#include "nova_surface_protocol.h"
#define NOVA_SURFACE_FEATURE_DESKTOP_OPEN 0x20000U
#define NOVA_SURFACE_EVENT_DESKTOP_OPEN 0x108U
/* Compositor-to-Files hint only. One Desktop basename, never a filesystem
 * grant, executable, arbitrary path or terminal command. */
static inline unsigned nova_desktop_open_name(const char*s){
 if(!s||!s[0]||s[0]=='.')return 0;
 for(unsigned i=0;i<NOVA_SURFACE_CLIPBOARD_BYTES;i++){unsigned c=(unsigned char)s[i];if(!c)return i;if(c<32||c==127||c=='/'||c=='\\')return 0;}return 0;
}
static inline int nova_desktop_open_event(const struct nova_surface_event*e){
 return e&&e->magic==NOVA_SURFACE_PROTOCOL_MAGIC&&e->version==6&&e->size==sizeof(*e)&&e->client_id==3&&e->surface_id==300&&e->type==NOVA_SURFACE_EVENT_DESKTOP_OPEN&&e->sequence&&e->clipboard_length&&e->clipboard_length<NOVA_SURFACE_CLIPBOARD_BYTES&&nova_desktop_open_name(e->clipboard)==e->clipboard_length&&!e->code&&!e->value&&!e->configure_serial&&!e->width&&!e->height&&!e->modifiers;
}
#endif
