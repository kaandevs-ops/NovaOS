#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "nova_tls_transport.h"
int main(int argc,char**argv){if(argc!=5)return 2;int fd=atoi(argv[1]),flags=fcntl(fd,F_GETFL);FILE*f=fopen(argv[3],"rb");if(!f)return 3;unsigned char ca[65536];size_t count=fread(ca,1,sizeof ca,f);fclose(f);struct nova_tls*t=NULL;int rc=nova_tls_open(&t,fd,argv[2],ca,count,(unsigned)atoi(argv[4]));size_t bytes=0;char reply[256]={0};
 if(!rc){char req[512];int n=snprintf(req,sizeof req,"GET /robots.txt HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",argv[2]);if(n<0||(size_t)n>=sizeof req)rc=-EINVAL;else rc=nova_tls_write(t,req,(size_t)n);if(!rc)rc=nova_tls_read(t,reply,sizeof reply-1,&bytes);}
 nova_tls_close(t);printf("{\"rc\":%d,\"bytes\":%zu,\"fd_preserved\":%s,\"http_ok\":%s}\n",rc,bytes,fcntl(fd,F_GETFL)==flags?"true":"false",bytes>=12&&reply[9]=='2'&&reply[10]=='0'&&reply[11]=='0'?"true":"false");return 0;}
