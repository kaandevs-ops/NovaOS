#include <stdio.h>
#include <arpa/inet.h>
#include "nova_net.h"
int main(void){
 struct nova_net_url u;
 const char *bad[]={"https://example.com/","http://127.0.0.1/","http://localhost/","http://user@example.com/","http://example.com:80/","http://-x.example/","http://x-.example/","http://example.com/\r\nInjected:x","http://example.com/#fragment","http://example.com/a b","http://example.com\\evil"};
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++)if(!nova_net_parse(bad[i],&u))return 1;
 if(nova_net_parse("http://example.com",&u)||strcmp(u.host,"example.com")||strcmp(u.path,"/"))return 2;
 if(nova_net_parse("http://example.com?q=1",&u)||strcmp(u.path,"/?q=1"))return 3;
 const char *ips[]={"0.0.0.0","10.0.2.2","100.64.0.1","127.0.0.1","169.254.169.254","172.16.0.1","192.168.1.1","192.0.2.1","198.18.0.1","198.51.100.1","203.0.113.1","224.0.0.1","255.255.255.255"};
 for(unsigned i=0;i<sizeof ips/sizeof ips[0];i++){struct in_addr a;inet_pton(AF_INET,ips[i],&a);if(nova_net_public(ntohl(a.s_addr)))return 4;}
 struct in_addr a;inet_pton(AF_INET,"93.184.216.34",&a);if(!nova_net_public(ntohl(a.s_addr)))return 5;
 puts("NETWORK_URL_AND_PUBLIC_IPV4_UNIT_PASS");return 0;
}
