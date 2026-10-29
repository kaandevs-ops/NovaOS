#define _POSIX_C_SOURCE 200809L
#include "nova_tls_transport.h"
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
struct nova_tls {SSL_CTX *ctx;SSL *ssl;BIO_METHOD *method;int fd,flags;int64_t deadline;};
/* MSG_NOSIGNAL is local to this socket write; no global signal changes. */
static int socket_create(BIO*b){BIO_set_init(b,1);BIO_set_data(b,NULL);return 1;}
static int socket_destroy(BIO*b){if(b)BIO_set_data(b,NULL);return 1;}
static int socket_read(BIO*b,char*out,int count){struct nova_tls*t=BIO_get_data(b);if(!t||!out||count<=0)return 0;BIO_clear_retry_flags(b);int n=(int)recv(t->fd,out,(size_t)count,0);if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))BIO_set_retry_read(b);return n;}
static int socket_write(BIO*b,const char*data,int count){struct nova_tls*t=BIO_get_data(b);if(!t||!data||count<=0)return 0;BIO_clear_retry_flags(b);int n=(int)send(t->fd,data,(size_t)count,MSG_NOSIGNAL);if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))BIO_set_retry_write(b);return n;}
static long socket_ctrl(BIO*b,int command,long num,void*ptr){(void)b;(void)num;(void)ptr;return command==BIO_CTRL_FLUSH?1:0;}
static int64_t milliseconds(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return -1;return (int64_t)t.tv_sec*1000+t.tv_nsec/1000000;}
static int wait_ssl(struct nova_tls*t,int result){
 int err=SSL_get_error(t->ssl,result);
 if(err!=SSL_ERROR_WANT_READ&&err!=SSL_ERROR_WANT_WRITE)return SSL_get_verify_result(t->ssl)!=X509_V_OK?-EACCES:-EPROTO;
 for(;;){int64_t now=milliseconds();if(now<0)return -EIO;if(now>=t->deadline)return -ETIMEDOUT;
 struct pollfd p={t->fd,err==SSL_ERROR_WANT_READ?POLLIN:POLLOUT,0};int n=poll(&p,1,(int)(t->deadline-now));
 if(n<0&&errno==EINTR)continue;if(n<0)return -errno;if(!n)return -ETIMEDOUT;if(p.revents&POLLNVAL)return -EBADF;return 0;}
}
void nova_tls_close(struct nova_tls*t){if(!t)return;SSL_free(t->ssl);SSL_CTX_free(t->ctx);BIO_meth_free(t->method);if(t->fd>=0&&t->flags>=0)(void)fcntl(t->fd,F_SETFL,t->flags);free(t);}
static int hostname_ok(const char*h){
 if(!h)return 0;size_t n=strnlen(h,254);if(!n||n>253)return 0;unsigned label=0;
 for(size_t i=0;i<n;i++){unsigned char c=h[i];if(c=='.'){if(!label||h[i-1]=='-')return 0;label=0;continue;}
 if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-')||(!label&&c=='-')||++label>63)return 0;}
 return label&&h[n-1]!='-';
}
int nova_tls_open(struct nova_tls**out,int fd,const char*hostname,const void*ca,size_t bytes,unsigned timeout){
 if(!out)return -EINVAL;*out=NULL;
 if(fd<0||!hostname_ok(hostname)||!ca||!bytes||bytes>1024U*1024U||!timeout||timeout>30000)return -EINVAL;
 if(!OPENSSL_init_ssl(OPENSSL_INIT_NO_LOAD_CONFIG,NULL))return -EIO;
 struct nova_tls*t=calloc(1,sizeof*t);if(!t)return -ENOMEM;t->fd=fd;t->flags=-1;
 t->flags=fcntl(fd,F_GETFL);if(t->flags<0){nova_tls_close(t);return -EBADF;}
 int64_t now=milliseconds();if(now<0){nova_tls_close(t);return -EIO;}t->deadline=now+timeout;
 t->ctx=SSL_CTX_new(TLS_client_method());if(!t->ctx){nova_tls_close(t);return -ENOMEM;}
 if(!SSL_CTX_set_min_proto_version(t->ctx,TLS1_2_VERSION)){nova_tls_close(t);return -EPROTO;}
 SSL_CTX_set_verify(t->ctx,SSL_VERIFY_PEER,NULL);SSL_CTX_set_verify_depth(t->ctx,8);
 BIO*b=BIO_new_mem_buf(ca,(int)bytes);if(!b){nova_tls_close(t);return -ENOMEM;}
 X509*cert;unsigned count=0;while((cert=PEM_read_bio_X509(b,NULL,NULL,NULL))){if(X509_STORE_add_cert(SSL_CTX_get_cert_store(t->ctx),cert))count++;X509_free(cert);}BIO_free(b);ERR_clear_error();
 if(!count){nova_tls_close(t);return -EINVAL;}
 t->ssl=SSL_new(t->ctx);const unsigned char alpn[]={8,'h','t','t','p','/','1','.','1'};
 if(!t->ssl||!SSL_set_tlsext_host_name(t->ssl,hostname)||!SSL_set1_host(t->ssl,hostname)||SSL_set_alpn_protos(t->ssl,alpn,sizeof alpn)||fcntl(fd,F_SETFL,t->flags|O_NONBLOCK)){nova_tls_close(t);return -EIO;}
 t->method=BIO_meth_new(BIO_TYPE_SOURCE_SINK|BIO_get_new_index(),"Nova authorized socket");
 if(!t->method||!BIO_meth_set_create(t->method,socket_create)||!BIO_meth_set_destroy(t->method,socket_destroy)||!BIO_meth_set_read(t->method,socket_read)||!BIO_meth_set_write(t->method,socket_write)||!BIO_meth_set_ctrl(t->method,socket_ctrl)){nova_tls_close(t);return -ENOMEM;}
 BIO*socket_bio=BIO_new(t->method);if(!socket_bio){nova_tls_close(t);return -ENOMEM;}BIO_set_data(socket_bio,t);SSL_set_bio(t->ssl,socket_bio,socket_bio);
 SSL_set_hostflags(t->ssl,X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
 int rc;for(;;){ERR_clear_error();int n=SSL_connect(t->ssl);if(n==1)break;rc=wait_ssl(t,n);if(rc){nova_tls_close(t);return rc;}}
 X509*peer=SSL_get1_peer_certificate(t->ssl);if(!peer||SSL_get_verify_result(t->ssl)!=X509_V_OK){X509_free(peer);nova_tls_close(t);return -EACCES;}X509_free(peer);
 *out=t;return 0;
}
int nova_tls_write(struct nova_tls*t,const void*data,size_t bytes){
 if(!t||(!data&&bytes)||bytes>1024U*1024U)return -EINVAL;
 size_t sent=0;while(sent<bytes){if(milliseconds()>=t->deadline)return -ETIMEDOUT;ERR_clear_error();int n=SSL_write(t->ssl,(const unsigned char*)data+sent,(int)(bytes-sent));if(n>0){sent+=(size_t)n;continue;}int rc=wait_ssl(t,n);if(rc)return rc;}return 0;
}
int nova_tls_read(struct nova_tls*t,void*data,size_t cap,size_t*bytes){
 if(bytes)*bytes=0;if(!t||!data||!bytes||!cap||cap>1024U*1024U)return -EINVAL;
 for(;;){if(milliseconds()>=t->deadline)return -ETIMEDOUT;ERR_clear_error();int n=SSL_read(t->ssl,data,(int)cap);if(n>0){*bytes=(size_t)n;return 0;}if(SSL_get_error(t->ssl,n)==SSL_ERROR_ZERO_RETURN)return 0;int rc=wait_ssl(t,n);if(rc)return rc;}
}
