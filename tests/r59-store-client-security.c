#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define nova_main unused_compositor_main
#include "../src/services/compositor.c"
#undef nova_main
static int peer;
static void setup(void){int f[2];assert(nova_syscall4(NOVA_TERM_NR_SOCKETPAIR,NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,0,(long)f)==0);nova_zero(&g_store,sizeof g_store);g_store.fd=f[0];peer=f[1];g_store.ready=1;g_store.pending=1;g_store.job=7;g_store.app=3;g_store.total=100;g_store.bytes=20;g_sw.needs_list=0;}
static void inject(struct nova_store_packet p,unsigned size){struct nova_iovec io={&p,size};struct nova_msghdr m;nova_zero(&m,sizeof m);m.msg_iov=&io;m.msg_iovlen=1;assert(nova_sendmsg(peer,&m,NOVA_MSG_NOSIGNAL)==size);assert(nova_store_receive());}
static void rejected(struct nova_store_packet p,unsigned size){setup();inject(p,size);assert(g_store.fd==-1&&g_store.error==71&&!g_store.ready&&!g_store.available&&!g_store.pending);assert(!g_sw.needs_list);nova_close(peer);}
int main(void){struct nova_store_packet p={NS_MAGIC,1,NS_PROGRESS,7,3,30,100,0};
 p.tag=8;rejected(p,sizeof p);p.tag=7;p.index=2;rejected(p,sizeof p);p.index=3;p.bytes=19;rejected(p,sizeof p);p.bytes=30;p.total=101;rejected(p,sizeof p);p.total=100;p.version=2;rejected(p,sizeof p);p.version=1;rejected(p,sizeof p-1);p.op=NS_READY;p.tag=0;p.index=0;p.bytes=0;p.total=0;p.code=8;rejected(p,sizeof p);
 p=(struct nova_store_packet){NS_MAGIC,1,NS_DONE,7,3,30,100,0};rejected(p,sizeof p);
 setup();p.bytes=100;inject(p,sizeof p);assert(!g_store.error&&!g_store.pending&&g_sw.needs_list);nova_store_down(5);nova_close(peer);
 setup();p.bytes=30;p.code=125;inject(p,sizeof p);assert(!g_store.error&&!g_store.pending&&g_store.result==125&&g_sw.needs_list);nova_store_down(5);nova_close(peer);
 puts("R59_CLIENT_MALFORMED_AND_STATE_TESTS_PASS: 8 rejected, success and cancellation accepted");return 0;}
