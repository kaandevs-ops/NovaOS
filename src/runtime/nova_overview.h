#ifndef NOVA_OVERVIEW_H
#define NOVA_OVERVIEW_H
#define NOVA_OVERVIEW_PATH "/run/nova/overview.sock"
#define NOVA_OVERVIEW_MAGIC 0x4e4f5652U
#define NOVA_OVERVIEW_VERSION 1U
struct nova_overview_request {unsigned int magic,version,size,sequence,reserved0,reserved1;};
struct nova_overview_reply {unsigned int magic,version,size,sequence;unsigned int services_total,services_running,apps_total,apps_running,restarts,secure_boot,tpm,verity,degraded,reserved;unsigned long long monotonic_ns;};
_Static_assert(sizeof(struct nova_overview_request)==24,"overview request ABI");
_Static_assert(sizeof(struct nova_overview_reply)==64,"overview reply ABI");
static inline int nova_overview_request_valid(const struct nova_overview_request*q){return q&&q->magic==NOVA_OVERVIEW_MAGIC&&q->version==1&&q->size==sizeof(*q)&&q->sequence&&q->reserved0==0&&q->reserved1==0;}
static inline int nova_overview_reply_valid(const struct nova_overview_reply*q,unsigned int seq){return q&&seq&&q->magic==NOVA_OVERVIEW_MAGIC&&q->version==1&&q->size==sizeof(*q)&&q->sequence==seq&&q->reserved==0&&q->services_total<=64&&q->services_running<=q->services_total&&q->apps_total<=q->services_total&&q->apps_running<=q->apps_total&&q->apps_running<=q->services_running&&q->secure_boot<=3&&q->tpm<=3&&q->verity<=3&&q->degraded<=1;}
#endif
