#ifndef NOVA_REPOSITORY_H
#define NOVA_REPOSITORY_H
#include <stddef.h>
#include <stdint.h>
#define NR_MAX_INDEX 65536U
#define NR_MAX_ENTRIES 64U
#define NR_MAX_PAYLOAD (8U*1024U*1024U)
struct nr_entry {char id[49],version[33],arch[8],sha256[65],url[513];uint64_t bytes;};
struct nr_index {uint64_t serial,expires;unsigned count;struct nr_entry entries[NR_MAX_ENTRIES];};
/* signature[64] || canonical ASCII index, Ed25519 public key pinned by caller. */
int nr_verify(const unsigned char*,size_t,const unsigned char[32],const char*,uint64_t,uint64_t,struct nr_index*);
int nr_publish_index(int,const unsigned char*,size_t,const unsigned char[32],const char*,uint64_t,uint64_t);
int nr_https_fetch(const struct nr_entry*,const char*,int);
#endif
