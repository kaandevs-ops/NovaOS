#ifndef NOVA_POWER_H
#define NOVA_POWER_H
#ifndef NOVA_POWER_PATH
#define NOVA_POWER_PATH "/run/nova/power.sock"
#endif
#define NOVA_POWER_MAGIC 0x4e505752U
struct nova_power_message {unsigned int magic,version,size,sequence,action,reserved;};
_Static_assert(sizeof(struct nova_power_message)==24,"power ABI");
static inline int nova_power_valid(const struct nova_power_message*q){return q&&q->magic==NOVA_POWER_MAGIC&&q->version==1&&q->size==sizeof(*q)&&q->sequence&&q->action==1&&!q->reserved;}
#endif
