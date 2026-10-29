#ifndef NOVA_SHA256_H
#define NOVA_SHA256_H

#include "nova_runtime.h"

struct nova_sha256_ctx {
    unsigned int state[8];
    unsigned long total_len;
    unsigned char block[64];
    nova_size_t block_len;
};

void nova_sha256_init(struct nova_sha256_ctx *ctx);
void nova_sha256_update(struct nova_sha256_ctx *ctx, const void *data, nova_size_t len);
void nova_sha256_final(struct nova_sha256_ctx *ctx, unsigned char out[32]);
void nova_sha256_hex(const unsigned char digest[32], char out[65]);
long nova_sha256_file_digest(const char *path, unsigned char out[32]);
long nova_sha256_file(const char *path, char out_hex[65]);
int nova_sha256_selftest(void);

#endif
