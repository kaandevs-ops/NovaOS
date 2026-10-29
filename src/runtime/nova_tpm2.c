#include "nova_tpm2.h"
#include "nova_runtime.h"

#define TPM2_ST_NO_SESSIONS 0x8001U
#define TPM2_ST_SESSIONS    0x8002U
#define TPM2_CC_NV_DEFINE_SPACE 0x0000012aUL
#define TPM2_CC_NV_INCREMENT    0x00000134UL
#define TPM2_CC_NV_WRITE        0x00000137UL
#define TPM2_CC_NV_WRITE_LOCK   0x00000138UL
#define TPM2_CC_NV_READ         0x0000014eUL
#define TPM2_CC_NV_READ_PUBLIC  0x00000169UL
#define TPM2_CC_PCR_READ        0x0000017eUL
#define TPM2_CC_PCR_EXTEND      0x00000182UL
#define TPM2_RH_OWNER           0x40000001UL
#define TPM2_RH_PW              0x40000009UL
#define TPM2_ALG_SHA256         0x000bU
#define TPMA_NV_OWNERWRITE      0x00000002UL
#define TPMA_NV_NT_MASK         0x000000f0UL
#define TPMA_NV_NT_COUNTER      0x00000010UL
#define TPMA_NV_WRITELOCKED     0x00000800UL
#define TPMA_NV_WRITEDEFINE     0x00002000UL
#define TPMA_NV_OWNERREAD       0x00020000UL
#define TPMA_NV_NO_DA           0x02000000UL
#define TPMA_NV_WRITTEN         0x20000000UL
#define TPM2_RC_SUCCESS         0x00000000UL
#define TPM2_RC_NV_UNINITIALIZED 0x0000014aUL

/*
 * TPM counter semantics matter here: a newly-defined TPMA_NV_COUNTER has no
 * readable value until its first TPM2_NV_Increment().  On first increment the
 * TPM may initialise the raw counter above *any* historical TPM counter value,
 * so NovaOS must not assume that the first readable raw value is 1.
 *
 * NovaOS therefore keeps a second, ordinary 8-byte NV index as a write-once
 * baseline.  Logical rollback generation 0 is represented by an uninitialised
 * counter + an unwritten baseline.  On the first confirmed update, the counter
 * is incremented, its device-specific raw value is copied to the baseline and
 * that baseline is permanently write-locked.  Thereafter:
 *
 *   logical_generation = 1 + (raw_counter - raw_baseline)
 *
 * This preserves device-independent signed rollback_index values 0,1,2,...
 * without assuming a TPM-specific initial raw counter value.
 */

static unsigned long g_tpm_last_rc = 0xffffffffUL;
static long g_tpm_last_io_status = 0;
static const char *g_tpm_last_pcr_source = "none";
static unsigned long g_tpm_last_raw_counter = 0;
static unsigned long g_tpm_last_counter_base = 0;
static int g_tpm_last_counter_initialized = 0;
static int g_tpm_last_baseline_written = 0;
static int g_tpm_last_baseline_locked = 0;

static void be16_put(unsigned char *p, unsigned int v) {
    p[0]=(unsigned char)((v>>8)&0xffU); p[1]=(unsigned char)(v&0xffU);
}
static void be32_put(unsigned char *p, unsigned long v) {
    p[0]=(unsigned char)((v>>24)&0xffUL); p[1]=(unsigned char)((v>>16)&0xffUL);
    p[2]=(unsigned char)((v>>8)&0xffUL); p[3]=(unsigned char)(v&0xffUL);
}
static void be64_put_ul(unsigned char *p, unsigned long v) {
    for(int i=7;i>=0;i--){ p[i]=(unsigned char)(v&0xffUL); v>>=8; }
}
static unsigned int be16_get(const unsigned char *p) {
    return ((unsigned int)p[0]<<8)|(unsigned int)p[1];
}
static unsigned long be32_get(const unsigned char *p) {
    return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|(unsigned long)p[3];
}
static unsigned long be64_get_ul(const unsigned char *p) {
    /* NovaOS currently targets 64-bit x86_64/aarch64 only. */
    unsigned long v=0;
    for(int i=0;i<8;i++) v=(v<<8)|(unsigned long)p[i];
    return v;
}

static long tpm_open(void) {
    long fd=nova_open_rw("/dev/tpmrm0");
    if(fd>=0) return fd;
    return nova_open_rw("/dev/tpm0");
}

int nova_tpm2_device_node_present(void) {
    long fd=tpm_open();
    if(fd<0) return 0;
    nova_close((int)fd);
    return 1;
}

static int tpm_xfer(const unsigned char *cmd, nova_size_t cmd_len,
                    unsigned char *rsp, nova_size_t rsp_cap, nova_size_t *rsp_len) {
    long fd=tpm_open();
    if(fd<0){g_tpm_last_io_status=fd;return 0;}
    long n;
    do { n=nova_write((int)fd,cmd,cmd_len); } while(n==-NOVA_EINTR);
    if(n!=(long)cmd_len){g_tpm_last_io_status=n;nova_close((int)fd);return 0;}
    do { n=nova_read((int)fd,rsp,rsp_cap); } while(n==-NOVA_EINTR);
    nova_close((int)fd);
    g_tpm_last_io_status=n;
    if(n<10) return 0;
    unsigned long declared=be32_get(rsp+2);
    if(declared<10 || declared>(unsigned long)n || declared>rsp_cap) return 0;
    g_tpm_last_rc=be32_get(rsp+6);
    if(rsp_len) *rsp_len=(nova_size_t)declared;
    return 1;
}

unsigned long nova_tpm2_last_rc(void) { return g_tpm_last_rc; }
long nova_tpm2_last_io_status(void) { return g_tpm_last_io_status; }
const char *nova_tpm2_last_pcr_source(void) { return g_tpm_last_pcr_source; }
unsigned long nova_tpm2_last_raw_counter(void) { return g_tpm_last_raw_counter; }
unsigned long nova_tpm2_last_counter_base(void) { return g_tpm_last_counter_base; }
int nova_tpm2_counter_initialized(void) { return g_tpm_last_counter_initialized; }
int nova_tpm2_baseline_written(void) { return g_tpm_last_baseline_written; }
int nova_tpm2_baseline_locked(void) { return g_tpm_last_baseline_locked; }
const char *nova_tpm2_rc_name(unsigned long rc) {
    if(rc==TPM2_RC_SUCCESS) return "SUCCESS";
    if(rc==TPM2_RC_NV_UNINITIALIZED) return "NV_UNINITIALIZED";
    return "OTHER";
}

int nova_tpm2_present(void) {
    long fd=nova_open_rw("/dev/tpmrm0");
    if(fd>=0){ nova_close((int)fd); return 1; }
    char b[8]; fd=nova_open_ro("/sys/class/tpm/tpm0/tpm_version_major");
    if(fd<0) return 0;
    long n;
    do { n=nova_read((int)fd,b,sizeof(b)); } while(n==-NOVA_EINTR);
    nova_close((int)fd);
    if(n<=0 || b[0]!='2') return 0;
    fd=nova_open_rw("/dev/tpm0");
    if(fd<0) return 0;
    nova_close((int)fd);
    return 1;
}

typedef struct {
    unsigned long index;
    unsigned int alg;
    unsigned long attrs;
    unsigned int data_size;
} nova_nv_public;

static int nv_read_public(unsigned long nv_index, nova_nv_public *out) {
    unsigned char cmd[14], rsp[160]; nova_size_t rn=0;
    be16_put(cmd,TPM2_ST_NO_SESSIONS); be32_put(cmd+2,sizeof(cmd)); be32_put(cmd+6,TPM2_CC_NV_READ_PUBLIC);
    be32_put(cmd+10,nv_index);
    if(!tpm_xfer(cmd,sizeof(cmd),rsp,sizeof(rsp),&rn) || g_tpm_last_rc!=TPM2_RC_SUCCESS) return 0;
    if(rn<28) return 0;
    unsigned int pub_size=be16_get(rsp+10);
    if(pub_size<14 || (unsigned long)(12U+pub_size)>rn) return 0;
    const unsigned char *p=rsp+12;
    nova_nv_public info;
    info.index=be32_get(p); p+=4;
    info.alg=be16_get(p); p+=2;
    info.attrs=be32_get(p); p+=4;
    unsigned int policy_size=be16_get(p); p+=2;
    if((unsigned long)(p-rsp)+policy_size+2UL>rn) return 0;
    p+=policy_size;
    info.data_size=be16_get(p);
    if(out) *out=info;
    return 1;
}

static int counter_public_valid(nova_nv_public *info_out) {
    nova_nv_public info;
    if(!nv_read_public(NOVA_TPM2_NV_ROLLBACK_INDEX,&info)) return 0;
    if(info.index!=NOVA_TPM2_NV_ROLLBACK_INDEX || info.alg!=TPM2_ALG_SHA256 ||
       (info.attrs&TPMA_NV_NT_MASK)!=TPMA_NV_NT_COUNTER ||
       (info.attrs&TPMA_NV_OWNERREAD)==0 || (info.attrs&TPMA_NV_OWNERWRITE)==0 || info.data_size!=8) return 0;
    if(info_out) *info_out=info;
    return 1;
}

static int baseline_public_valid(nova_nv_public *info_out) {
    nova_nv_public info;
    if(!nv_read_public(NOVA_TPM2_NV_ROLLBACK_BASE_INDEX,&info)) return 0;
    if(info.index!=NOVA_TPM2_NV_ROLLBACK_BASE_INDEX || info.alg!=TPM2_ALG_SHA256 ||
       (info.attrs&TPMA_NV_NT_MASK)!=0 ||
       (info.attrs&TPMA_NV_OWNERREAD)==0 || (info.attrs&TPMA_NV_OWNERWRITE)==0 ||
       (info.attrs&TPMA_NV_WRITEDEFINE)==0 || info.data_size!=8) return 0;
    if(info_out) *info_out=info;
    return 1;
}

static int nv_define_index(unsigned long index, unsigned long attrs) {
    unsigned char cmd[64], rsp[64]; nova_size_t o=0,rn=0;
    be16_put(cmd+o,TPM2_ST_SESSIONS); o+=2; o+=4; be32_put(cmd+o,TPM2_CC_NV_DEFINE_SPACE); o+=4;
    be32_put(cmd+o,TPM2_RH_OWNER); o+=4;
    be32_put(cmd+o,9); o+=4; be32_put(cmd+o,TPM2_RH_PW); o+=4; be16_put(cmd+o,0); o+=2; cmd[o++]=0; be16_put(cmd+o,0); o+=2;
    be16_put(cmd+o,0); o+=2; /* authValue */
    be16_put(cmd+o,14); o+=2;
    be32_put(cmd+o,index); o+=4;
    be16_put(cmd+o,TPM2_ALG_SHA256); o+=2;
    be32_put(cmd+o,attrs); o+=4;
    be16_put(cmd+o,0); o+=2;
    be16_put(cmd+o,8); o+=2;
    be32_put(cmd+2,(unsigned long)o);
    if(!tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn)) return 0;
    return g_tpm_last_rc==TPM2_RC_SUCCESS;
}

static int nv_define_counter(void) {
    return nv_define_index(NOVA_TPM2_NV_ROLLBACK_INDEX,
        TPMA_NV_OWNERWRITE|TPMA_NV_NT_COUNTER|TPMA_NV_OWNERREAD|TPMA_NV_NO_DA);
}

static int nv_define_baseline(void) {
    return nv_define_index(NOVA_TPM2_NV_ROLLBACK_BASE_INDEX,
        TPMA_NV_OWNERWRITE|TPMA_NV_WRITEDEFINE|TPMA_NV_OWNERREAD|TPMA_NV_NO_DA);
}

static int nv_read_u64(unsigned long index, unsigned long *value) {
    unsigned char cmd[64], rsp[128]; nova_size_t o=0,rn=0;
    be16_put(cmd+o,TPM2_ST_SESSIONS); o+=2; o+=4; be32_put(cmd+o,TPM2_CC_NV_READ); o+=4;
    be32_put(cmd+o,TPM2_RH_OWNER); o+=4; be32_put(cmd+o,index); o+=4;
    be32_put(cmd+o,9); o+=4; be32_put(cmd+o,TPM2_RH_PW); o+=4; be16_put(cmd+o,0); o+=2; cmd[o++]=0; be16_put(cmd+o,0); o+=2;
    be16_put(cmd+o,8); o+=2; be16_put(cmd+o,0); o+=2;
    be32_put(cmd+2,(unsigned long)o);
    if(!tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn) || g_tpm_last_rc!=TPM2_RC_SUCCESS) return 0;
    if(rn<24 || be16_get(rsp)!=TPM2_ST_SESSIONS) return 0;
    unsigned long parameter_size=be32_get(rsp+10);
    if(parameter_size<10 || 14UL+parameter_size>rn) return 0;
    if(be16_get(rsp+14)!=8) return 0;
    if(value) *value=be64_get_ul(rsp+16);
    return 1;
}

static int nv_write_u64(unsigned long index, unsigned long value) {
    unsigned char cmd[72], rsp[64]; nova_size_t o=0,rn=0;
    be16_put(cmd+o,TPM2_ST_SESSIONS); o+=2; o+=4; be32_put(cmd+o,TPM2_CC_NV_WRITE); o+=4;
    be32_put(cmd+o,TPM2_RH_OWNER); o+=4; be32_put(cmd+o,index); o+=4;
    be32_put(cmd+o,9); o+=4; be32_put(cmd+o,TPM2_RH_PW); o+=4; be16_put(cmd+o,0); o+=2; cmd[o++]=0; be16_put(cmd+o,0); o+=2;
    be16_put(cmd+o,8); o+=2; be64_put_ul(cmd+o,value); o+=8;
    be16_put(cmd+o,0); o+=2;
    be32_put(cmd+2,(unsigned long)o);
    if(!tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn)) return 0;
    return g_tpm_last_rc==TPM2_RC_SUCCESS;
}

static int nv_write_lock(unsigned long index) {
    unsigned char cmd[48], rsp[64]; nova_size_t o=0,rn=0;
    be16_put(cmd+o,TPM2_ST_SESSIONS); o+=2; o+=4; be32_put(cmd+o,TPM2_CC_NV_WRITE_LOCK); o+=4;
    be32_put(cmd+o,TPM2_RH_OWNER); o+=4; be32_put(cmd+o,index); o+=4;
    be32_put(cmd+o,9); o+=4; be32_put(cmd+o,TPM2_RH_PW); o+=4; be16_put(cmd+o,0); o+=2; cmd[o++]=0; be16_put(cmd+o,0); o+=2;
    be32_put(cmd+2,(unsigned long)o);
    if(!tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn)) return 0;
    return g_tpm_last_rc==TPM2_RC_SUCCESS;
}

static int nv_increment_raw(void) {
    unsigned char cmd[48], rsp[64]; nova_size_t o=0,rn=0;
    be16_put(cmd+o,TPM2_ST_SESSIONS); o+=2; o+=4; be32_put(cmd+o,TPM2_CC_NV_INCREMENT); o+=4;
    be32_put(cmd+o,TPM2_RH_OWNER); o+=4; be32_put(cmd+o,NOVA_TPM2_NV_ROLLBACK_INDEX); o+=4;
    be32_put(cmd+o,9); o+=4; be32_put(cmd+o,TPM2_RH_PW); o+=4; be16_put(cmd+o,0); o+=2; cmd[o++]=0; be16_put(cmd+o,0); o+=2;
    be32_put(cmd+2,(unsigned long)o);
    if(!tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn)) return 0;
    return g_tpm_last_rc==TPM2_RC_SUCCESS;
}

static int ensure_counter_and_baseline(int *created) {
    nova_nv_public base_info, counter_info;
    int base_ok=baseline_public_valid(&base_info);
    if(!base_ok) {
        if(!nv_define_baseline()) return 0; /* existing wrong index also fails closed here */
        if(created) *created=1;
        if(!baseline_public_valid(&base_info)) return 0;
    }
    int counter_ok=counter_public_valid(&counter_info);
    if(!counter_ok) {
        if(!nv_define_counter()) return 0;
        if(created) *created=1;
        if(!counter_public_valid(&counter_info)) return 0;
    }
    return 1;
}

int nova_tpm2_prepare_rollback_counter(unsigned long *value, int *created) {
    if(created) *created=0;
    g_tpm_last_raw_counter=0; g_tpm_last_counter_base=0;
    g_tpm_last_counter_initialized=0; g_tpm_last_baseline_written=0; g_tpm_last_baseline_locked=0;
    if(!nova_tpm2_present()) return 0;
    if(!ensure_counter_and_baseline(created)) return 0;

    nova_nv_public counter_info, base_info;
    if(!counter_public_valid(&counter_info) || !baseline_public_valid(&base_info)) return 0;
    int counter_written=(counter_info.attrs&TPMA_NV_WRITTEN)!=0;
    int base_written=(base_info.attrs&TPMA_NV_WRITTEN)!=0;
    int base_locked=(base_info.attrs&TPMA_NV_WRITELOCKED)!=0;
    g_tpm_last_counter_initialized=counter_written;
    g_tpm_last_baseline_written=base_written;
    g_tpm_last_baseline_locked=base_locked;

    if(!counter_written) {
        /* Fresh counter: TPM2_NV_Read would return TPM_RC_NV_UNINITIALIZED.
         * That is NovaOS logical generation 0, not a boot failure. */
        if(base_written) return 0; /* impossible/inconsistent provisioning state */
        /* TPMA_NV_WRITTEN clear is the expected logical generation-0 state. */
        g_tpm_last_rc=TPM2_RC_SUCCESS;
        if(value) *value=0;
        return 1;
    }

    unsigned long raw=0;
    if(!nv_read_u64(NOVA_TPM2_NV_ROLLBACK_INDEX,&raw)) return 0;
    g_tpm_last_raw_counter=raw;

    if(!base_written) {
        /* Power-loss recovery for the very first confirmed update: the TPM
         * increment is durable but the baseline write/lock did not complete. */
        if(!nv_write_u64(NOVA_TPM2_NV_ROLLBACK_BASE_INDEX,raw)) return 0;
        if(!nv_write_lock(NOVA_TPM2_NV_ROLLBACK_BASE_INDEX)) return 0;
        if(created) *created=1;
        if(!baseline_public_valid(&base_info)) return 0;
        base_written=(base_info.attrs&TPMA_NV_WRITTEN)!=0;
        base_locked=(base_info.attrs&TPMA_NV_WRITELOCKED)!=0;
        if(!base_written || !base_locked) return 0;
    } else if(!base_locked) {
        /* Finish an interrupted baseline lock before trusting it. */
        if(!nv_write_lock(NOVA_TPM2_NV_ROLLBACK_BASE_INDEX)) return 0;
        if(!baseline_public_valid(&base_info) || (base_info.attrs&TPMA_NV_WRITELOCKED)==0) return 0;
        base_locked=1;
    }

    unsigned long base=0;
    if(!nv_read_u64(NOVA_TPM2_NV_ROLLBACK_BASE_INDEX,&base)) return 0;
    if(raw<base || raw-base==(~0UL)) return 0;
    g_tpm_last_counter_base=base;
    g_tpm_last_baseline_written=1;
    g_tpm_last_baseline_locked=base_locked;
    unsigned long logical=1UL+(raw-base);
    if(value) *value=logical;
    return 1;
}

int nova_tpm2_read_rollback_counter(unsigned long *value) {
    int created=0;
    return nova_tpm2_prepare_rollback_counter(value,&created);
}

int nova_tpm2_increment_rollback_counter(unsigned long expected_current, unsigned long *new_value) {
    unsigned long before=0;
    int created=0;
    if(!nova_tpm2_prepare_rollback_counter(&before,&created) || before!=expected_current || before==~0UL) return 0;
    if(!nv_increment_raw()) return 0;
    unsigned long after=0;
    if(!nova_tpm2_prepare_rollback_counter(&after,&created) || after!=before+1UL) return 0;
    if(new_value) *new_value=after;
    return 1;
}

int nova_tpm2_pcr_extend_sha256(unsigned int pcr, const unsigned char digest[32]) {
    if(pcr>23 || !digest) return 0;
    unsigned char cmd[96], rsp[64]; nova_size_t o=0,rn=0;
    be16_put(cmd+o,TPM2_ST_SESSIONS); o+=2; o+=4; be32_put(cmd+o,TPM2_CC_PCR_EXTEND); o+=4;
    be32_put(cmd+o,(unsigned long)pcr); o+=4;
    be32_put(cmd+o,9); o+=4; be32_put(cmd+o,TPM2_RH_PW); o+=4; be16_put(cmd+o,0); o+=2; cmd[o++]=0; be16_put(cmd+o,0); o+=2;
    be32_put(cmd+o,1); o+=4; be16_put(cmd+o,TPM2_ALG_SHA256); o+=2;
    for(int i=0;i<32;i++) cmd[o++]=digest[i];
    be32_put(cmd+2,(unsigned long)o);
    if(!tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn)) return 0;
    return g_tpm_last_rc==TPM2_RC_SUCCESS;
}

static char hex_digit(unsigned int v) {
    return (char)(v<10U ? ('0'+v) : ('a'+(v-10U)));
}

static void digest_to_hex(const unsigned char *digest, nova_size_t len, char out[65]) {
    nova_size_t j=0;
    for(nova_size_t i=0;i<len && i<32;i++) {
        out[j++]=hex_digit((unsigned int)((digest[i]>>4)&0x0fU));
        out[j++]=hex_digit((unsigned int)(digest[i]&0x0fU));
    }
    out[j]='\0';
}

static int hex_value(char c) {
    if(c>='0' && c<='9') return (int)(c-'0');
    if(c>='a' && c<='f') return 10+(int)(c-'a');
    if(c>='A' && c<='F') return 10+(int)(c-'A');
    return -1;
}

static int append_dec_u32(char *dst, nova_size_t cap, nova_size_t *used, unsigned int value) {
    char tmp[10]; nova_size_t n=0;
    if(!dst || !used || *used>=cap) return 0;
    if(value==0U) tmp[n++]='0';
    while(value>0U && n<sizeof(tmp)){ tmp[n++]=(char)('0'+(value%10U)); value/=10U; }
    if(*used+n+1U>cap) return 0;
    while(n>0U) dst[(*used)++]=tmp[--n];
    dst[*used]='\0';
    return 1;
}

static int build_pcr_sysfs_path(unsigned int pcr, char *path, nova_size_t cap) {
    static const char prefix[]="/sys/class/tpm/tpm0/pcr-sha256/";
    if(!path || pcr>23U || cap<sizeof(prefix)+2U) return 0;
    nova_size_t used=0;
    for(nova_size_t i=0;prefix[i];i++) path[used++]=prefix[i];
    path[used]='\0';
    return append_dec_u32(path,cap,&used,pcr);
}

static int read_pcr_sha256_sysfs(unsigned int pcr, char hex_out[65]) {
    char path[48];
    if(!hex_out || !build_pcr_sysfs_path(pcr,path,sizeof(path))) return 0;
    char raw[96]; nova_size_t used=0;
    long fd=nova_open_ro(path);
    if(fd<0){g_tpm_last_io_status=fd;return 0;}
    while(used+1U<sizeof(raw)) {
        long n;
        do { n=nova_read((int)fd,raw+used,sizeof(raw)-used-1U); } while(n==-NOVA_EINTR);
        if(n<0){g_tpm_last_io_status=n;nova_close((int)fd);hex_out[0]='\0';return 0;}
        if(n==0) break;
        used+=(nova_size_t)n;
    }
    nova_close((int)fd); g_tpm_last_io_status=(long)used;
    raw[used]='\0';
    if(used<64U) return 0;
    for(int i=0;i<64;i++) {
        int v=hex_value(raw[i]);
        if(v<0){ hex_out[0]='\0'; return 0; }
        hex_out[i]=hex_digit((unsigned int)v);
    }
    for(nova_size_t i=64U;i<used;i++) {
        if(raw[i]!='\n' && raw[i]!='\r' && raw[i]!=' ' && raw[i]!='\t'){ hex_out[0]='\0'; return 0; }
    }
    hex_out[64]='\0';
    g_tpm_last_pcr_source="sysfs";
    return 1;
}

static int read_pcr_sha256_direct(unsigned int pcr, char hex_out[65]) {
    unsigned char cmd[32], rsp[256]; nova_size_t o=0,rn=0;
    if(pcr>23U || !hex_out) return 0;
    be16_put(cmd+o,TPM2_ST_NO_SESSIONS); o+=2;
    o+=4;
    be32_put(cmd+o,TPM2_CC_PCR_READ); o+=4;
    be32_put(cmd+o,1); o+=4;
    be16_put(cmd+o,TPM2_ALG_SHA256); o+=2;
    cmd[o++]=3;
    cmd[o++]=0; cmd[o++]=0; cmd[o++]=0;
    cmd[o-3U+(pcr/8U)] = (unsigned char)(1U<<(pcr%8U));
    be32_put(cmd+2,(unsigned long)o);

    if(!tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn) || g_tpm_last_rc!=TPM2_RC_SUCCESS) return 0;
    nova_size_t at=10U;
    if(at+4U>rn) return 0; /* updateCounter */
    at+=4U;
    if(at+4U>rn) return 0;
    unsigned long selections=be32_get(rsp+at); at+=4U;
    if(selections<1UL || selections>8UL) return 0;
    int requested_seen=0;
    for(unsigned long i=0;i<selections;i++) {
        if(at+3U>rn) return 0;
        unsigned int alg=be16_get(rsp+at); at+=2U;
        unsigned int sz=(unsigned int)rsp[at++];
        if(sz==0U || sz>8U || at+(nova_size_t)sz>rn) return 0;
        if(alg==TPM2_ALG_SHA256 && pcr/8U<sz &&
           (rsp[at+(pcr/8U)]&(unsigned char)(1U<<(pcr%8U)))) requested_seen=1;
        at+=(nova_size_t)sz;
    }
    if(!requested_seen || at+4U>rn) return 0;
    unsigned long digests=be32_get(rsp+at); at+=4U;
    if(digests<1UL || digests>8UL) return 0;
    if(at+2U>rn) return 0;
    unsigned int dsz=be16_get(rsp+at); at+=2U;
    if(dsz!=32U || at+32U>rn) return 0;
    digest_to_hex(rsp+at,32U,hex_out);
    g_tpm_last_pcr_source="direct";
    return 1;
}


int nova_tpm2_read_pcr_sha256_sysfs_only(unsigned int pcr, char hex_out[65]) {
    if(!hex_out || pcr>23U) return 0;
    hex_out[0]='\0'; g_tpm_last_pcr_source="none";
    return read_pcr_sha256_sysfs(pcr,hex_out);
}

int nova_tpm2_read_pcr_sha256_direct_only(unsigned int pcr, char hex_out[65]) {
    if(!hex_out || pcr>23U) return 0;
    hex_out[0]='\0'; g_tpm_last_pcr_source="none";
    return read_pcr_sha256_direct(pcr,hex_out);
}
int nova_tpm2_read_pcr_sha256(unsigned int pcr, char hex_out[65]) {
    if(!hex_out || pcr>23U) return 0;
    hex_out[0]='\0'; g_tpm_last_pcr_source="none";
    /* Prefer Linux's stable TPM PCR sysfs ABI.  The kernel TPM core performs
     * the PCR_Read transaction and exposes only allocated banks.  Keep our
     * minimal raw TPM2_PCR_Read codec as a fallback and independent check. */
    if(read_pcr_sha256_sysfs(pcr,hex_out)) return 1;
    return read_pcr_sha256_direct(pcr,hex_out);
}

/* Stage-4D release-trust epoch anti-rollback state.
 *
 * This deliberately uses an independent TPM NV counter/baseline pair rather
 * than overloading the System rollback generation.  A logical trust counter N
 * represents minimum accepted release-policy trust_epoch N+1.  The same
 * arbitrary-raw TPM counter mapping and first-increment power-loss recovery
 * used by the Stage-4C rollback counter are retained here.
 */
static int trust_counter_public_valid(nova_nv_public *info_out) {
    nova_nv_public info;
    if(!nv_read_public(NOVA_TPM2_NV_TRUST_EPOCH_INDEX,&info)) return 0;
    if(info.index!=NOVA_TPM2_NV_TRUST_EPOCH_INDEX || info.alg!=TPM2_ALG_SHA256 ||
       (info.attrs&TPMA_NV_NT_MASK)!=TPMA_NV_NT_COUNTER ||
       (info.attrs&TPMA_NV_OWNERREAD)==0 || (info.attrs&TPMA_NV_OWNERWRITE)==0 || info.data_size!=8) return 0;
    if(info_out) *info_out=info;
    return 1;
}

static int trust_baseline_public_valid(nova_nv_public *info_out) {
    nova_nv_public info;
    if(!nv_read_public(NOVA_TPM2_NV_TRUST_BASE_INDEX,&info)) return 0;
    if(info.index!=NOVA_TPM2_NV_TRUST_BASE_INDEX || info.alg!=TPM2_ALG_SHA256 ||
       (info.attrs&TPMA_NV_NT_MASK)!=0 ||
       (info.attrs&TPMA_NV_OWNERREAD)==0 || (info.attrs&TPMA_NV_OWNERWRITE)==0 ||
       (info.attrs&TPMA_NV_WRITEDEFINE)==0 || info.data_size!=8) return 0;
    if(info_out) *info_out=info;
    return 1;
}

static int trust_nv_define_counter(void) {
    return nv_define_index(NOVA_TPM2_NV_TRUST_EPOCH_INDEX,
        TPMA_NV_OWNERWRITE|TPMA_NV_NT_COUNTER|TPMA_NV_OWNERREAD|TPMA_NV_NO_DA);
}

static int trust_nv_define_baseline(void) {
    return nv_define_index(NOVA_TPM2_NV_TRUST_BASE_INDEX,
        TPMA_NV_OWNERWRITE|TPMA_NV_WRITEDEFINE|TPMA_NV_OWNERREAD|TPMA_NV_NO_DA);
}

static int trust_nv_increment_raw(void) {
    unsigned char cmd[48], rsp[64]; nova_size_t o=0,rn=0;
    be16_put(cmd+o,TPM2_ST_SESSIONS); o+=2; o+=4; be32_put(cmd+o,TPM2_CC_NV_INCREMENT); o+=4;
    be32_put(cmd+o,TPM2_RH_OWNER); o+=4; be32_put(cmd+o,NOVA_TPM2_NV_TRUST_EPOCH_INDEX); o+=4;
    be32_put(cmd+o,9); o+=4; be32_put(cmd+o,TPM2_RH_PW); o+=4; be16_put(cmd+o,0); o+=2; cmd[o++]=0; be16_put(cmd+o,0); o+=2;
    be32_put(cmd+2,(unsigned long)o);
    if(!tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn)) return 0;
    return g_tpm_last_rc==TPM2_RC_SUCCESS;
}

static int ensure_trust_counter_and_baseline(int *created) {
    nova_nv_public base_info, counter_info;
    int base_ok=trust_baseline_public_valid(&base_info);
    if(!base_ok) {
        if(!trust_nv_define_baseline()) return 0;
        if(created) *created=1;
        if(!trust_baseline_public_valid(&base_info)) return 0;
    }
    int counter_ok=trust_counter_public_valid(&counter_info);
    if(!counter_ok) {
        if(!trust_nv_define_counter()) return 0;
        if(created) *created=1;
        if(!trust_counter_public_valid(&counter_info)) return 0;
    }
    return 1;
}

int nova_tpm2_prepare_trust_epoch_counter(unsigned long *value, int *created) {
    if(created) *created=0;
    if(!nova_tpm2_present()) return 0;
    if(!ensure_trust_counter_and_baseline(created)) return 0;
    nova_nv_public counter_info, base_info;
    if(!trust_counter_public_valid(&counter_info) || !trust_baseline_public_valid(&base_info)) return 0;
    int counter_written=(counter_info.attrs&TPMA_NV_WRITTEN)!=0;
    int base_written=(base_info.attrs&TPMA_NV_WRITTEN)!=0;
    int base_locked=(base_info.attrs&TPMA_NV_WRITELOCKED)!=0;
    if(!counter_written) {
        if(base_written) return 0;
        g_tpm_last_rc=TPM2_RC_SUCCESS;
        if(value) *value=0;
        return 1;
    }
    unsigned long raw=0;
    if(!nv_read_u64(NOVA_TPM2_NV_TRUST_EPOCH_INDEX,&raw)) return 0;
    if(!base_written) {
        if(!nv_write_u64(NOVA_TPM2_NV_TRUST_BASE_INDEX,raw)) return 0;
        if(!nv_write_lock(NOVA_TPM2_NV_TRUST_BASE_INDEX)) return 0;
        if(created) *created=1;
        if(!trust_baseline_public_valid(&base_info)) return 0;
        base_written=(base_info.attrs&TPMA_NV_WRITTEN)!=0;
        base_locked=(base_info.attrs&TPMA_NV_WRITELOCKED)!=0;
        if(!base_written || !base_locked) return 0;
    } else if(!base_locked) {
        if(!nv_write_lock(NOVA_TPM2_NV_TRUST_BASE_INDEX)) return 0;
        if(!trust_baseline_public_valid(&base_info) || (base_info.attrs&TPMA_NV_WRITELOCKED)==0) return 0;
        base_locked=1;
    }
    unsigned long base=0;
    if(!nv_read_u64(NOVA_TPM2_NV_TRUST_BASE_INDEX,&base)) return 0;
    if(raw<base || raw-base==(~0UL)) return 0;
    if(value) *value=1UL+(raw-base);
    return 1;
}

int nova_tpm2_read_trust_epoch_counter(unsigned long *value) {
    int created=0;
    return nova_tpm2_prepare_trust_epoch_counter(value,&created);
}

int nova_tpm2_increment_trust_epoch_counter(unsigned long expected_current, unsigned long *new_value) {
    unsigned long before=0; int created=0;
    if(!nova_tpm2_prepare_trust_epoch_counter(&before,&created) || before!=expected_current || before==~0UL) return 0;
    if(!trust_nv_increment_raw()) return 0;
    unsigned long after=0;
    if(!nova_tpm2_prepare_trust_epoch_counter(&after,&created) || after!=before+1UL) return 0;
    if(new_value) *new_value=after;
    return 1;
}
