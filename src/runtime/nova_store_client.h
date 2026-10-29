#ifndef NOVA_STORE_CLIENT_H
#define NOVA_STORE_CLIENT_H
#include "nova_store_protocol.h"
struct nova_store_client {int fd;long child;unsigned ready,available,tag,job,app,pending,bytes,total,result,error;};
static unsigned r59_store_queue;
static struct nova_store_client g_store={.fd=-1};
static void nova_store_down(unsigned error){r59_store_queue=0;if(g_store.fd>=0)nova_close(g_store.fd);g_store.fd=-1;g_store.ready=0;g_store.available=0;g_store.pending=0;g_store.error=error?error:5;}
static void nova_store_reap(void){if(g_store.child>0){int status;long p=nova_wait4(g_store.child,&status,NOVA_WNOHANG);if(p==g_store.child||p==-10)g_store.child=0;}}
static int nova_store_start(void){nova_store_reap();if(g_store.fd>=0)return 1;if(g_store.child>0)return 0;
 int f[2];if(nova_syscall4(NOVA_TERM_NR_SOCKETPAIR,NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK|0x80000,0,(long)f)<0)return 0;long p=nova_clone_process();if(p<0){nova_close(f[0]);nova_close(f[1]);return 0;}
 if(!p){nova_close(f[0]);if(f[1]!=3){if(nova_syscall3(NOVA_TERM_NR_DUP3,f[1],3,0)<0)nova_exit(126);}else if(nova_syscall3(NOVA_TERM_NR_FCNTL,3,2,0)<0)nova_exit(126);if(nova_close_range(4,~0U,0)<0)nova_exit(126);for(int i=0;i<3;i++)nova_close(i);char*args[]={NOVA_STORE_HELPER,0};char*env[]={0};nova_execve(args[0],args,env);nova_exit(127);}
 nova_close(f[1]);g_store.fd=f[0];g_store.child=p;g_store.ready=0;g_store.error=0;g_store.result=0;g_store.pending=0;g_store.tag=0;g_store.job=0;g_store.app=0;g_store.bytes=0;g_store.total=0;g_store.available=0;return 1;}
static int nova_store_request(unsigned op,unsigned app){if(!g_store.ready||g_store.fd<0||app>=4)return 0;if(op==NS_FETCH&&(g_store.pending||!(g_store.available&(1U<<app))))return 0;if(op==NS_CANCEL&&(g_store.pending!=1||app!=g_store.app))return 0;
 if(op==NS_CANCEL)r59_store_queue=0;
 struct nova_store_packet q={NS_MAGIC,1,op,g_store.tag+1,app,0,0,0};struct nova_iovec io={&q,sizeof q};struct nova_msghdr msg;nova_zero(&msg,sizeof msg);msg.msg_iov=&io;msg.msg_iovlen=1;if(!nova_store_valid(&q,sizeof q,1)||nova_sendmsg(g_store.fd,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_NOSIGNAL)!=sizeof q)return 0;
 g_store.tag=q.tag;if(op==NS_FETCH){g_store.job=q.tag;g_store.app=app;g_store.pending=1;g_store.bytes=0;g_store.total=0;g_store.result=0;}else g_store.pending=2;return 1;}
static int r59_store_next(void){if(g_store.pending||!g_store.ready)return 0;for(unsigned i=0;i<4;i++)if(r59_store_queue&(1U<<i)){r59_store_queue&=~(1U<<i);if(nova_store_request(NS_FETCH,i))return 1;r59_store_queue=0;return 0;}return 0;}
static int r59_store_install_available(void){if(!g_store.ready||!g_sw.ready||g_store.pending||g_sw.pending)return 0;r59_store_queue=0;for(unsigned i=0;i<4;i++)if((g_store.available&(1U<<i))&&g_sw.installed[i]==3)r59_store_queue|=1U<<i;return r59_store_next();}
static int nova_store_receive(void){int changed=0;for(unsigned i=0;i<4;i++){struct nova_store_packet p;struct nova_iovec io={&p,sizeof p};struct nova_msghdr msg;nova_zero(&msg,sizeof msg);msg.msg_iov=&io;msg.msg_iovlen=1;long n=nova_recvmsg(g_store.fd,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_TRUNC);if(n==-11||n==-4)break;if(n<=0||msg.msg_flags||!nova_store_valid(&p,(unsigned long)n,0)){nova_store_down(71);return 1;}
 if(p.op==NS_READY){if(g_store.ready){nova_store_down(71);return 1;}g_store.ready=1;g_store.available=p.code;changed=1;continue;}
 if(!g_store.ready||!g_store.pending||p.tag!=g_store.job||p.index!=g_store.app||(g_store.total&&p.total!=g_store.total)){nova_store_down(71);return 1;}
 if(p.op==NS_PROGRESS){if(p.bytes<g_store.bytes){nova_store_down(71);return 1;}g_store.bytes=p.bytes;g_store.total=p.total;}
 else{g_store.pending=0;g_store.bytes=p.bytes;g_store.total=p.total;g_store.result=p.code;g_sw.needs_list=1;if(p.code)r59_store_queue=0;else (void)r59_store_next();}changed=1;
 }return changed;}
static void nova_store_status(char*out){const char*text=g_store.error?"Depo bağlantısı kesildi; mağazayı yeniden açarak deneyin.":g_store.pending==2?"İndirme iptal ediliyor…":g_store.pending?"Paket indiriliyor; ardından doğrulanacak…":g_store.result==125?"İndirme iptal edildi; mevcut uygulamalar korundu.":g_store.result==74?"Dosya doğrulaması başarısız; kurulum yapılmadı.":g_store.result==110?"İndirme zaman aşımına uğradı; yeniden deneyebilirsiniz.":g_store.result==62?"Depo indeksinin süresi dolmuş; güncelleme gerekiyor.":g_store.result==17?"Uygulama zaten kurulu; liste güncelleniyor.":g_store.result?"İndirme veya doğrulama başarısız; mevcut uygulama korundu.":g_store.job?"Kurulum tamamlandı; Kurulu bölümünden açabilirsiniz.":g_store.ready&&g_store.available?"İmzalı depo · HTTPS ve SHA-256 doğrulaması": "Geçerli internet deposu yok; yerel katalog kullanılabilir.";
 nova_copy(out,text,nova_strlen(text)+1);if(g_store.pending==1&&g_store.total){unsigned at=(unsigned)nova_strlen(out);out[at++]=' ';unsigned values[]={g_store.bytes/1024,g_store.total/1024};for(unsigned n=0;n<2;n++){char d[12];unsigned used=0,v=values[n];do{d[used++]=(char)('0'+v%10);v/=10;}while(v);while(used)out[at++]=d[--used];out[at++]=n? ' ':'/';}const char*unit="KiB";for(unsigned k=0;unit[k];k++)out[at++]=unit[k];out[at]=0;}}
#endif
