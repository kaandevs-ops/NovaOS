#include "nova_tcg2.h"
#include "nova_runtime.h"
#include "nova_sha256.h"

#define TCG2_EVENT_LOG_MAX (1024U * 1024U)
#define TCG2_EVENT_COUNT_MAX 65536U
#define TCG2_MAX_ALGS 16U
#define TCG2_ALG_SHA256 0x000bU
#define TCG2_EV_NO_ACTION 0x00000003UL

struct tcg2_alg_size { unsigned int alg; unsigned int size; };
#ifndef NOVA_TCG2_PARSER_ONLY
static unsigned char g_tcg2_log[TCG2_EVENT_LOG_MAX + 1U];
#endif
static const char *g_tcg2_error="none";

static unsigned int le16(const unsigned char *p){ return (unsigned int)p[0] | ((unsigned int)p[1]<<8); }
static unsigned long le32(const unsigned char *p){ return (unsigned long)p[0] | ((unsigned long)p[1]<<8) | ((unsigned long)p[2]<<16) | ((unsigned long)p[3]<<24); }
static int bounds(unsigned long at,unsigned long need,unsigned long total){ return at<=total && need<=total-at; }
static unsigned int digest_size_for(const struct tcg2_alg_size *algs,unsigned int n,unsigned int alg){
    for(unsigned int i=0;i<n;i++) if(algs[i].alg==alg) return algs[i].size;
    return 0;
}
static void clear_evidence(struct nova_tcg2_evidence *out){
    unsigned char *p=(unsigned char *)out; for(unsigned long i=0;i<(unsigned long)sizeof(*out);i++)p[i]=0;
}

int nova_tcg2_parse_event_log(const unsigned char *log,unsigned long total,struct nova_tcg2_evidence *out){
    if(!out){g_tcg2_error="invalid-argument";return 0;}
    clear_evidence(out);
    if(!log){g_tcg2_error="invalid-argument";return 0;}

    /* Preserve a digest and exact byte count for rejected-but-readable logs
     * inside the configured resource bound. These fields are diagnostic
     * evidence only; replay remains untrusted unless the complete parser
     * succeeds and the caller matches it to live PCR7. */
    out->log_bytes=total;
    if(total>TCG2_EVENT_LOG_MAX){g_tcg2_error="event-log-too-large";return 0;}
    {
        unsigned char log_digest[32]; struct nova_sha256_ctx lctx;
        nova_sha256_init(&lctx); nova_sha256_update(&lctx,log,total); nova_sha256_final(&lctx,log_digest);
        nova_sha256_hex(log_digest,out->log_sha256);
    }
    if(total<32U){g_tcg2_error="event-log-short";return 0;}

    unsigned long off=0;
    if(le32(log)!=0UL || le32(log+4)!=TCG2_EV_NO_ACTION){g_tcg2_error="specid-legacy-header";return 0;}
    for(unsigned int i=0;i<20U;i++) if(log[8U+i]!=0U){g_tcg2_error="specid-legacy-digest";return 0;}
    unsigned long legacy_event_size=le32(log+28); off=32;
    if(!bounds(off,legacy_event_size,total) || legacy_event_size<29U){g_tcg2_error="specid-short";return 0;}
    const unsigned char *spec=log+off;
    const char sig[]="Spec ID Event03";
    for(unsigned int i=0;i<15U;i++) if(spec[i]!=(unsigned char)sig[i]){g_tcg2_error="specid-signature";return 0;}
    if(spec[15]!=0){g_tcg2_error="specid-terminator";return 0;}
    unsigned long s=24U;
    if(!bounds(s,4,legacy_event_size)){g_tcg2_error="specid-alg-count";return 0;}
    unsigned long alg_count=le32(spec+s); s+=4;
    if(alg_count==0 || alg_count>TCG2_MAX_ALGS){g_tcg2_error="specid-alg-range";return 0;}
    struct tcg2_alg_size algs[TCG2_MAX_ALGS]; int has_sha256=0;
    for(unsigned long i=0;i<alg_count;i++){
        if(!bounds(s,4,legacy_event_size)){g_tcg2_error="specid-alg-entry";return 0;}
        algs[i].alg=le16(spec+s); algs[i].size=le16(spec+s+2); s+=4;
        if(algs[i].size==0 || algs[i].size>64U){g_tcg2_error="specid-digest-size";return 0;}
        for(unsigned long j=0;j<i;j++) if(algs[j].alg==algs[i].alg){g_tcg2_error="specid-duplicate-alg";return 0;}
        if(algs[i].alg==TCG2_ALG_SHA256){
            if(algs[i].size!=32U){g_tcg2_error="sha256-size-invalid";return 0;}
            has_sha256=1;
        }
    }
    if(!has_sha256){g_tcg2_error="sha256-bank-not-described";return 0;}
    if(!bounds(s,1,legacy_event_size)){g_tcg2_error="specid-vendor-size";return 0;}
    unsigned int vendor_size=spec[s++];
    if(!bounds(s,vendor_size,legacy_event_size)){g_tcg2_error="specid-vendor-data";return 0;}
    s+=vendor_size;
    if(s!=legacy_event_size){g_tcg2_error="specid-trailing-data";return 0;}
    off+=legacy_event_size;

    unsigned char replay[32]; for(unsigned int i=0;i<32U;i++)replay[i]=0;
    unsigned long event_count=0,pcr7_count=0;
    while(off<total){
        if(!bounds(off,12,total)){g_tcg2_error="event2-header-short";return 0;}
        unsigned long pcr=le32(log+off); unsigned long event_type=le32(log+off+4);
        unsigned long digests=le32(log+off+8); off+=12;
        if(digests==0 || digests>TCG2_MAX_ALGS){g_tcg2_error="event2-digest-count";return 0;}
        unsigned char sha256_digest[32]; int have_sha256=0; unsigned int seen_algs[TCG2_MAX_ALGS];
        for(unsigned long i=0;i<digests;i++){
            if(!bounds(off,2,total)){g_tcg2_error="event2-alg-short";return 0;}
            unsigned int alg=le16(log+off); off+=2;
            for(unsigned long j=0;j<i;j++) if(seen_algs[j]==alg){g_tcg2_error="event2-duplicate-alg";return 0;}
            seen_algs[i]=alg;
            unsigned int dsz=digest_size_for(algs,(unsigned int)alg_count,alg);
            if(dsz==0 || !bounds(off,dsz,total)){g_tcg2_error="event2-digest-short";return 0;}
            if(alg==TCG2_ALG_SHA256 && dsz==32U){for(unsigned int j=0;j<32U;j++)sha256_digest[j]=log[off+j];have_sha256=1;}
            off+=dsz;
        }
        if(!bounds(off,4,total)){g_tcg2_error="event2-size-short";return 0;}
        unsigned long event_size=le32(log+off); off+=4;
        if(!bounds(off,event_size,total)){g_tcg2_error="event2-data-short";return 0;}
        off+=event_size;
        if(event_count>=TCG2_EVENT_COUNT_MAX){g_tcg2_error="event-count-limit";return 0;}
        if(pcr>23UL){g_tcg2_error="event2-pcr-range";return 0;}
        event_count++; out->event2_count=event_count;
        if(pcr==7UL && have_sha256 && event_type!=TCG2_EV_NO_ACTION){
            struct nova_sha256_ctx ctx; unsigned char next[32];
            nova_sha256_init(&ctx); nova_sha256_update(&ctx,replay,32); nova_sha256_update(&ctx,sha256_digest,32); nova_sha256_final(&ctx,next);
            for(unsigned int j=0;j<32U;j++) replay[j]=next[j];
            pcr7_count++;
            out->pcr7_sha256_events=pcr7_count;
            nova_sha256_hex(replay,out->replayed_pcr7_sha256);
        }
    }
    if(pcr7_count==0){g_tcg2_error="pcr7-events-missing";return 0;}
    nova_sha256_hex(replay,out->replayed_pcr7_sha256);
    out->event2_count=event_count; out->pcr7_sha256_events=pcr7_count;
    g_tcg2_error="none"; return 1;
}

#ifndef NOVA_TCG2_PARSER_ONLY
static int read_log(const char *path,unsigned long *len_out){
    long fd=nova_open_ro(path); if(fd<0){g_tcg2_error="event-log-open";return 0;}
    unsigned long used=0;
    for(;;){
        long n=nova_read((int)fd,g_tcg2_log+used,(nova_size_t)((TCG2_EVENT_LOG_MAX+1U)-used));
        if(n==-NOVA_EINTR) continue;
        if(n<0){nova_close((int)fd);g_tcg2_error="event-log-read";return 0;}
        if(n==0) break;
        used+=(unsigned long)n;
        if(used>TCG2_EVENT_LOG_MAX){nova_close((int)fd);g_tcg2_error="event-log-too-large";return 0;}
    }
    nova_close((int)fd);
    *len_out=used; return 1;
}
int nova_tcg2_collect_evidence(const char *path,struct nova_tcg2_evidence *out){
    if(!out){g_tcg2_error="invalid-argument";return 0;}
    clear_evidence(out);
    unsigned long total=0; if(!read_log(path,&total)) return 0;
    return nova_tcg2_parse_event_log(g_tcg2_log,total,out);
}
#endif

const char *nova_tcg2_last_error(void){return g_tcg2_error;}
