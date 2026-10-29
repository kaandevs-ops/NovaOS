#ifndef NOVA_NET_H
#define NOVA_NET_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define NOVA_NET_MAGIC 0x4e485450U
#define NOVA_NET_URL 1024U
#define NOVA_NET_BODY 65536U
#define NOVA_NET_FD 4
struct nova_net_header {uint32_t magic,version,tag,length;int32_t code;};
struct nova_net_url {char host[254],path[1026];};
/* Deliberately HTTP-only. TLS, authentication and redirects are not faked. */
static inline int nova_net_parse(const char *url,struct nova_net_url *out){
 size_t n=strlen(url);if(n<8||n>NOVA_NET_URL||strncmp(url,"http://",7))return -1;
 for(size_t i=0;i<n;i++){unsigned char c=url[i];if(c<=32||c>=127||c=='\\'||c=='#')return -1;}
 const char *a=url+7,*end=strpbrk(a,"/?");if(!end)end=url+n;
 size_t hn=(size_t)(end-a);if(!hn||hn>=sizeof out->host)return -1;
 unsigned label=0,dots=0,alpha=0;
 for(size_t i=0;i<hn;i++){unsigned char c=a[i];
  if(c=='.'){if(!label||a[i-1]=='-')return -1;label=0;dots++;continue;}
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'))return -1;
  if(!label&&c=='-')return -1;
  if(++label>63)return -1;
  if((c>='a'&&c<='z')||(c>='A'&&c<='Z'))alpha=1;
 }
 if(!label||a[hn-1]=='-'||!dots||!alpha)return -1;
 memcpy(out->host,a,hn);out->host[hn]=0;
 if(!*end)strcpy(out->path,"/");else if(*end=='?'){out->path[0]='/';strcpy(out->path+1,end);}else strcpy(out->path,end);
 return 0;
}
/* Host-order IPv4. Conservative rejection of non-public/special ranges. */
static inline int nova_net_public(uint32_t ip){
 unsigned a=ip>>24,b=(ip>>16)&255,c=(ip>>8)&255;
 if(!a||a==10||a==127||a>=224)return 0;
 if(a==100&&b>=64&&b<=127)return 0;
 if(a==169&&b==254)return 0;
 if(a==172&&b>=16&&b<=31)return 0;
 if(a==192&&(b==168||(b==0&&(c==0||c==2))||(b==88&&c==99)))return 0;
 if(a==198&&(b==18||b==19||(b==51&&c==100)))return 0;
 if(a==203&&b==0&&c==113)return 0;
 return 1;
}
#endif
