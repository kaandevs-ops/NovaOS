#ifndef NOVA_THEME_H
#define NOVA_THEME_H
#include "nova_surface_protocol.h"
#define NOVA_SURFACE_FEATURE_THEME 0x10000U
#define NOVA_SURFACE_EVENT_THEME 0x107U
/* Read-only notification state. Only trusted compositor input changes authority.
 * Event/message sizes and v6 opcodes stay unchanged; no client SET preference. */
struct nova_theme_state { unsigned int choice,generation; };
static inline int nova_theme_accept(struct nova_theme_state*s,const struct nova_surface_event*e,unsigned int client,unsigned int first,unsigned int second){
 if(!s||!e||e->magic!=NOVA_SURFACE_PROTOCOL_MAGIC||e->version!=6||e->size!=sizeof(*e)||e->type!=NOVA_SURFACE_EVENT_THEME||e->client_id!=client||!e->surface_id||(e->surface_id!=first&&e->surface_id!=second)||e->code>2||e->value<=0||(unsigned int)e->value<=s->generation||e->configure_serial||e->width||e->height||e->clipboard_length)return 0;
 s->choice=e->code;s->generation=(unsigned int)e->value;return 1;
}
#endif
