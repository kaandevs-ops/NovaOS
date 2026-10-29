#ifndef NOVA_TLS_TRANSPORT_H
#define NOVA_TLS_TRANSPORT_H
#include <stddef.h>
struct nova_tls;
/* Caller supplies an already authorized connected socket and immutable PEM CA.
 * No DNS, sockets, files, redirects or principal changes are performed here.
 * fd stays caller-owned; only one thread may use fd until close. */
int nova_tls_open(struct nova_tls **out,int fd,const char *hostname,const void *ca,size_t ca_bytes,unsigned timeout_ms);
int nova_tls_write(struct nova_tls *t,const void *data,size_t bytes);
int nova_tls_read(struct nova_tls *t,void *data,size_t capacity,size_t *bytes);
void nova_tls_close(struct nova_tls *t);
#endif
