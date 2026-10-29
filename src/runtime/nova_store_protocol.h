#ifndef NOVA_STORE_PROTOCOL_H
#define NOVA_STORE_PROTOCOL_H
#define NS_MAGIC 0x4e535452U
#define NS_VERSION 1U
#ifndef NOVA_STORE_HELPER
#define NOVA_STORE_HELPER "/System/Core/nova-store-session"
#endif
enum {NS_FETCH=1,NS_CANCEL=2,NS_READY=16,NS_PROGRESS=17,NS_DONE=18};
struct nova_store_packet {unsigned magic,version,op,tag,index,bytes,total,code;};
static inline int nova_store_valid(const struct nova_store_packet*p,unsigned long n,int request){
 if(!p||n!=sizeof *p||p->magic!=NS_MAGIC||p->version!=NS_VERSION)return 0;
 if(request)return p->tag&&p->index<4&&!p->bytes&&!p->total&&!p->code&&(p->op==NS_FETCH||p->op==NS_CANCEL);
 if(p->op==NS_READY)return !p->tag&&!p->index&&!p->bytes&&!p->total&&p->code<=15;
 return p->tag&&p->index<4&&p->total<=8388608&&p->bytes<=p->total&&((p->op==NS_PROGRESS&&!p->code)||(p->op==NS_DONE&&p->code<=4095&&(p->code||p->bytes==p->total)));
}
_Static_assert(sizeof(struct nova_store_packet)==32,"store wire bound");
#endif
