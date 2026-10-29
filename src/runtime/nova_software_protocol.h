#ifndef NOVA_SOFTWARE_PROTOCOL_H
#define NOVA_SOFTWARE_PROTOCOL_H
#define NOVA_SW_MAGIC 0x4e535746U
#define NOVA_SW_VERSION 1U
#define NOVA_SW_BYTES 1024U
#define NOVA_SW_COUNT 4U
#ifndef NOVA_SOFTWARE_HELPER
#define NOVA_SOFTWARE_HELPER "/System/Core/nova-software-session"
#endif
enum {NOVA_SW_LIST=1,NOVA_SW_INSTALL=2,NOVA_SW_REMOVE=3,NOVA_SW_RUN=4,NOVA_SW_INPUT=5,NOVA_SW_RESIZE=6,NOVA_SW_CLOSE=7,NOVA_SW_READY=16,NOVA_SW_STATE=17,NOVA_SW_DATA=18,NOVA_SW_EXIT=19};
struct nova_sw_packet{unsigned magic,version,op,tag,index,length,code,reserved;unsigned char data[NOVA_SW_BYTES];};
struct nova_sw_state{unsigned installed[4],running,result,action,app;};
static const char*const nova_sw_ids[]={"sqlite","lua","lua-dynamic","2048"};
static const char*const nova_sw_titles[]={"SQLite","Lua","Lua Dynamic","2048"};
static const char*const nova_sw_descriptions[]={"Yerel SQL veritabanı ve komut ortamı","Lua programlama ve betik ortamı","Paylaşımlı runtime kullanan Lua ortamı","Gerçek, çevrimdışı sayı bulmacası"};
static inline int nova_sw_valid(const struct nova_sw_packet*p,unsigned long n,int request){
 if(!p||n<32||p->magic!=NOVA_SW_MAGIC||p->version!=1||p->reserved||p->length>NOVA_SW_BYTES||n!=32UL+p->length)return 0;
 if(request){if(!p->tag||p->index>=4||p->code)return 0;
  if(p->op==NOVA_SW_INPUT)return p->length&&p->length<=512;
  if(p->op==NOVA_SW_RESIZE){if(p->length!=8)return 0;const unsigned*v=(const void*)p->data;return v[0]>=4&&v[0]<=48&&v[1]>=20&&v[1]<=120;}
  return p->op>=NOVA_SW_LIST&&p->op<=NOVA_SW_CLOSE&&p->op!=NOVA_SW_INPUT&&p->op!=NOVA_SW_RESIZE&&!p->length&&(p->op!=NOVA_SW_LIST||!p->index);
 }
 if(p->op==NOVA_SW_READY)return !p->tag&&!p->index&&!p->length&&p->code==12004;
 if(p->index>=4)return 0;
 if(p->op==NOVA_SW_DATA)return !p->tag&&!p->code&&p->length;
 if(p->op==NOVA_SW_EXIT)return !p->tag&&!p->length&&p->code<=255;
 if(p->op==NOVA_SW_STATE){if(!p->tag||p->code||p->length!=sizeof(struct nova_sw_state))return 0;const struct nova_sw_state*s=(const void*)p->data;if(s->running>15||s->result>4095||s->action<1||s->action>7||s->app>=4)return 0;for(unsigned i=0;i<4;i++)if(s->installed[i]>4||s->installed[i]==2)return 0;return 1;}
 return 0;
}
_Static_assert(sizeof(struct nova_sw_packet)==1056,"software packet bound");
#endif
