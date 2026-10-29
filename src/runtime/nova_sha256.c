#include "nova_sha256.h"

static unsigned int rotr32(unsigned int x, unsigned int n) {
    return (x >> n) | (x << (32U - n));
}

static unsigned int load_be32(const unsigned char *p) {
    return ((unsigned int)p[0] << 24) |
           ((unsigned int)p[1] << 16) |
           ((unsigned int)p[2] << 8) |
           (unsigned int)p[3];
}

static void store_be32(unsigned char *p, unsigned int x) {
    p[0] = (unsigned char)(x >> 24);
    p[1] = (unsigned char)(x >> 16);
    p[2] = (unsigned char)(x >> 8);
    p[3] = (unsigned char)x;
}

static const unsigned int k256[64] = {
    0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
    0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
    0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
    0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
    0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
    0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
    0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
    0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
};

static void transform(struct nova_sha256_ctx *ctx, const unsigned char block[64]) {
    unsigned int w[64];
    for (int i = 0; i < 16; i++) w[i] = load_be32(block + (i * 4));
    for (int i = 16; i < 64; i++) {
        unsigned int s0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >> 3);
        unsigned int s1 = rotr32(w[i-2], 17) ^ rotr32(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    unsigned int a=ctx->state[0], b=ctx->state[1], c=ctx->state[2], d=ctx->state[3];
    unsigned int e=ctx->state[4], f=ctx->state[5], g=ctx->state[6], h=ctx->state[7];
    for (int i = 0; i < 64; i++) {
        unsigned int s1 = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);
        unsigned int ch = (e & f) ^ ((~e) & g);
        unsigned int t1 = h + s1 + ch + k256[i] + w[i];
        unsigned int s0 = rotr32(a,2) ^ rotr32(a,13) ^ rotr32(a,22);
        unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
        unsigned int t2 = s0 + maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    ctx->state[0]+=a; ctx->state[1]+=b; ctx->state[2]+=c; ctx->state[3]+=d;
    ctx->state[4]+=e; ctx->state[5]+=f; ctx->state[6]+=g; ctx->state[7]+=h;
}

void nova_sha256_init(struct nova_sha256_ctx *ctx) {
    ctx->state[0]=0x6a09e667U; ctx->state[1]=0xbb67ae85U; ctx->state[2]=0x3c6ef372U; ctx->state[3]=0xa54ff53aU;
    ctx->state[4]=0x510e527fU; ctx->state[5]=0x9b05688cU; ctx->state[6]=0x1f83d9abU; ctx->state[7]=0x5be0cd19U;
    ctx->total_len=0; ctx->block_len=0;
}

void nova_sha256_update(struct nova_sha256_ctx *ctx, const void *data, nova_size_t len) {
    const unsigned char *p = (const unsigned char *)data;
    ctx->total_len += len;
    while (len) {
        nova_size_t room = 64 - ctx->block_len;
        nova_size_t take = len < room ? len : room;
        for (nova_size_t i=0;i<take;i++) ctx->block[ctx->block_len+i]=p[i];
        ctx->block_len += take; p += take; len -= take;
        if (ctx->block_len == 64) { transform(ctx, ctx->block); ctx->block_len = 0; }
    }
}

void nova_sha256_final(struct nova_sha256_ctx *ctx, unsigned char out[32]) {
    unsigned long bit_len = ctx->total_len * 8UL;
    ctx->block[ctx->block_len++] = 0x80;
    if (ctx->block_len > 56) {
        while (ctx->block_len < 64) ctx->block[ctx->block_len++] = 0;
        transform(ctx, ctx->block); ctx->block_len = 0;
    }
    while (ctx->block_len < 56) ctx->block[ctx->block_len++] = 0;
    for (int i=7;i>=0;i--) ctx->block[ctx->block_len++] = (unsigned char)(bit_len >> (i*8));
    transform(ctx, ctx->block);
    for (int i=0;i<8;i++) store_be32(out + i*4, ctx->state[i]);
}

void nova_sha256_hex(const unsigned char digest[32], char out[65]) {
    static const char hex[] = "0123456789abcdef";
    for (int i=0;i<32;i++) { out[i*2]=hex[digest[i]>>4]; out[i*2+1]=hex[digest[i]&15]; }
    out[64]='\0';
}

long nova_sha256_file_digest(const char *path, unsigned char out[32]) {
    long fd = nova_open_ro(path);
    if (fd < 0) return fd;
    struct nova_sha256_ctx ctx; nova_sha256_init(&ctx);
    unsigned char buf[1024];
    for (;;) {
        long n = nova_read((int)fd, buf, sizeof(buf));
        if (n < 0) { nova_close((int)fd); return n; }
        if (n == 0) break;
        nova_sha256_update(&ctx, buf, (nova_size_t)n);
    }
    nova_close((int)fd);
    nova_sha256_final(&ctx, out);
    return 0;
}

long nova_sha256_file(const char *path, char out_hex[65]) {
    unsigned char digest[32];
    long rc = nova_sha256_file_digest(path, digest);
    if (rc < 0) return rc;
    nova_sha256_hex(digest, out_hex);
    return 0;
}

int nova_sha256_selftest(void) {
    static const char expected[] = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    struct nova_sha256_ctx ctx; unsigned char digest[32]; char hex[65];
    nova_sha256_init(&ctx); nova_sha256_update(&ctx, "abc", 3); nova_sha256_final(&ctx, digest); nova_sha256_hex(digest, hex);
    return nova_streq(hex, expected);
}
