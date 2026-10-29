#include "nova_overview.h"
#include "nova_power.h"
#include "nova_runtime.h"
#include "nova_sha256.h"
#include "nova_tpm2.h"
#include "nova_tcg2.h"
#include "nova_app_protocol.h"
#include "nova_app_catalog_protocol.h"
#include "nova_app_metadata_protocol.h"
#include "nova_app_security_protocol.h"
#include "nova_app_sandbox.h"
#include "nova_files_grant.h"
#include "nova_resource_broker_protocol.h"
#include "nova_surface_protocol.h"

#ifndef NOVA_VERSION
#define NOVA_VERSION "0.0.12-dev"
#endif
#ifndef NOVA_BUILD_ARCH
#define NOVA_BUILD_ARCH "unknown"
#endif
#ifndef NOVA_ROOT_CERT_SHA256
#define NOVA_ROOT_CERT_SHA256 "UNPINNED"
#endif
#ifndef NOVA_TRUST_PROFILE
#define NOVA_TRUST_PROFILE "development"
#endif
#ifndef NOVA_TRUST_KEY_ID
#define NOVA_TRUST_KEY_ID "nova-development-root-2026"
#endif
#ifndef NOVA_UEFI_CERT_SHA256
#define NOVA_UEFI_CERT_SHA256 "UNPINNED"
#endif
#ifndef NOVA_RELEASE_KEY_ID
#define NOVA_RELEASE_KEY_ID "nova-development-release-2026-a"
#endif
#ifndef NOVA_TRUST_EPOCH
#define NOVA_TRUST_EPOCH 1UL
#endif

#define INPUT_MAX 256
#define FILE_BUFFER 4096
#if defined(NOVA_R47_BROWSER_COMPAT) && NOVA_R47_BROWSER_COMPAT
#define INTEGRITY_BUFFER 131072
#else
#define INTEGRITY_BUFFER 16384
#endif
#define TRUST_BUFFER 4096
#define SIGNATURE_BUFFER 1024
#define MAX_SERVICES 16
#define SERVICE_NAME_MAX 32
#define SERVICE_PATH_MAX 160
#define INTEGRITY_MANIFEST "/System/Integrity/manifest.sha256"
#define INTEGRITY_SIGNATURE "/System/Integrity/manifest.sig"
#define ROOT_CERT_PATH "/System/Trust/nova-root.der"
#define PREBOOT_ROOT_CERT_PATH "/Trust/nova-root.der"
#define NOVA_UEFI_POLICY_PATH "/Trust/uefi-boot.conf"
#define NOVA_EFIVARS_DIR "/run/efivars"
#define NOVA_EFI_SECUREBOOT_VAR "/run/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c"
#define NOVA_EFI_SETUPMODE_VAR "/run/efivars/SetupMode-8be4df61-93ca-11d2-aa0d-00e098032b8c"
#define NOVA_DATA_DEV (g_nova_storage_paths[3])
#define NOVA_SYSTEM_A_DEV (g_nova_storage_paths[0])
#define NOVA_SYSTEM_B_DEV (g_nova_storage_paths[1])
#define NOVA_RECOVERY_DEV (g_nova_storage_paths[2])
#define NOVA_SLOT_METADATA_LEGACY "/Data/Nova/boot-slot.conf"
#define NOVA_BOOT_STATE_DIR "/Data/Nova/Boot"
#define NOVA_SLOT_METADATA_A "/Data/Nova/Boot/state.A"
#define NOVA_SLOT_METADATA_A_TMP "/Data/Nova/Boot/state.A.new"
#define NOVA_SLOT_METADATA_B "/Data/Nova/Boot/state.B"
#define NOVA_SLOT_METADATA_B_TMP "/Data/Nova/Boot/state.B.new"
#define NOVA_PERSIST_FILE "/Data/Nova/persistence-test.txt"
#define NOVA_PERSIST_TMP "/Data/Nova/persistence-test.txt.new"
#define NOVA_UPDATE_DIR "/Data/Nova/Update"
#define NOVA_UPDATE_MANIFEST "/Data/Nova/Update/update.manifest"
#define NOVA_UPDATE_SIGNATURE "/Data/Nova/Update/update.sig"
#define NOVA_UPDATE_PAYLOAD "/Data/Nova/Update/system.slot"
#if defined(NOVA_R47_BROWSER_COMPAT) && NOVA_R47_BROWSER_COMPAT
#define NOVA_SYSTEM_SLOT_BYTES (520UL*1024UL*1024UL)
#define NOVA_VERITY_DATA_BLOCKS 131072UL
#define NOVA_VERITY_HASH_START_BLOCK 131072UL
#else
#define NOVA_SYSTEM_SLOT_BYTES 50331648UL
#define NOVA_VERITY_DATA_BLOCKS 10240UL
#define NOVA_VERITY_HASH_START_BLOCK 10240UL
#endif
#define NOVA_VERITY_DATA_BLOCK_SIZE 4096UL
#define NOVA_VERITY_HASH_BLOCK_SIZE 4096UL
#define NOVA_DM_CONTROL "/dev/mapper/control"
#define NOVA_DM_NODE "/dev/mapper/nova-system"
#define NOVA_RECOVERY_DM_NODE "/dev/mapper/nova-recovery"
#define NOVA_RECOVERY_SLOT_BYTES 25165824UL
#define NOVA_RECOVERY_VERITY_DATA_BLOCKS 5120UL
#define NOVA_RECOVERY_VERITY_HASH_START_BLOCK 5120UL
#define NOVA_RECOVERY_VERITY_MANIFEST_EMBEDDED "/Trust/recovery-verity.manifest"
#define NOVA_RECOVERY_VERITY_SIGNATURE_EMBEDDED "/Trust/recovery-verity.sig"
#define NOVA_RECOVERY_VERITY_MANIFEST "/Data/Nova/Recovery/verity.manifest"
#define NOVA_RECOVERY_VERITY_SIGNATURE "/Data/Nova/Recovery/verity.sig"
#define NOVA_SLOT_A_VERITY_MANIFEST "/Data/Nova/Slots/A/verity.manifest"
#define NOVA_SLOT_A_VERITY_SIGNATURE "/Data/Nova/Slots/A/verity.sig"
#define NOVA_SLOT_B_VERITY_MANIFEST "/Data/Nova/Slots/B/verity.manifest"
#define NOVA_SLOT_B_VERITY_SIGNATURE "/Data/Nova/Slots/B/verity.sig"
#define NOVA_PENDING_KIND_MAX 16
#define NOVA_UPDATE_ID_MAX 64
#define NOVA_UPDATE_VERSION_MAX 64
#define NOVA_MEASUREMENTS_LOG "/run/nova/measurements.log"
#define NOVA_EVIDENCE_FILE "/run/nova/boot-evidence.env"
#define NOVA_TCG_EVENT_LOG "/sys/kernel/security/tpm0/binary_bios_measurements"
#define NOVA_RELEASE_POLICY "/System/Trust/release-policy.conf"
#define NOVA_RELEASE_POLICY_SIGNATURE "/System/Trust/release-policy.sig"
#define NOVA_RELEASE_CERT "/System/Trust/release-active.der"
#define NOVA_RELEASE_NEXT_CERT "/System/Trust/release-next.der"
#define NOVA_UPDATE_RELEASE_POLICY "/Data/Nova/Update/release-policy.conf"
#define NOVA_UPDATE_RELEASE_POLICY_SIGNATURE "/Data/Nova/Update/release-policy.sig"
#define NOVA_UPDATE_RELEASE_CERT "/Data/Nova/Update/release-active.der"
#define NOVA_UPDATE_RELEASE_NEXT_CERT "/Data/Nova/Update/release-next.der"
#define NOVA_SLOT_A_TRUST_DIR "/Data/Nova/Slots/A/Trust"
#define NOVA_SLOT_A_RELEASE_POLICY "/Data/Nova/Slots/A/Trust/release-policy.conf"
#define NOVA_SLOT_A_RELEASE_POLICY_SIGNATURE "/Data/Nova/Slots/A/Trust/release-policy.sig"
#define NOVA_SLOT_A_RELEASE_CERT "/Data/Nova/Slots/A/Trust/release-active.der"
#define NOVA_SLOT_A_RELEASE_NEXT_CERT "/Data/Nova/Slots/A/Trust/release-next.der"
#define NOVA_SLOT_B_TRUST_DIR "/Data/Nova/Slots/B/Trust"
#define NOVA_SLOT_B_RELEASE_POLICY "/Data/Nova/Slots/B/Trust/release-policy.conf"
#define NOVA_SLOT_B_RELEASE_POLICY_SIGNATURE "/Data/Nova/Slots/B/Trust/release-policy.sig"
#define NOVA_SLOT_B_RELEASE_CERT "/Data/Nova/Slots/B/Trust/release-active.der"
#define NOVA_APP_STOP_GRACE_NS 2000000000ULL
#define NOVA_APP_STOP_TERM_NS 1000000000ULL
#define NOVA_APP_STOP_GRACE_FALLBACK_TICKS 20U
#define NOVA_APP_STOP_TERM_FALLBACK_TICKS 10U
#define NOVA_APP_STATE_FILE "/run/nova/app-lifecycle.state"
#define NOVA_APP_CATALOG_STATE_FILE "/run/nova/app-catalog.state"
#define NOVA_APP_SECURITY_STATE_FILE "/run/nova/app-security.state"
#define NOVA_SLOT_B_RELEASE_NEXT_CERT "/Data/Nova/Slots/B/Trust/release-next.der"

struct nova_service {
    char name[SERVICE_NAME_MAX];
    char exec[SERVICE_PATH_MAX];
    char app_id[NOVA_APP_ID_BYTES];
    char display_name[NOVA_APP_DISPLAY_NAME_BYTES];
    char description[NOVA_APP_DESCRIPTION_BYTES];
    char icon_asset_id[NOVA_APP_ICON_ASSET_ID_BYTES];
    unsigned int client_id;
    unsigned int category;
    unsigned long long declared_capabilities;
    unsigned long long effective_capabilities;
    unsigned int sandbox_profile;
    int files_user_home;
    unsigned long long sandbox_required_flags;
    int enabled;
    int critical;
    int restart_always;
    int autostart;
    int is_application;
    int desired_running;
    long pid;
    unsigned long restart_count;
    unsigned int app_state;
    unsigned long long app_generation;
    unsigned long long stop_deadline_ns;
    unsigned int stop_fallback_ticks;
    unsigned int stop_signal_stage;
};

enum trust_state {
    TRUST_UNKNOWN = 0,
    TRUST_VERIFIED,
    TRUST_FAILED,
    TRUST_UNAVAILABLE
};

enum integrity_state {
    INTEGRITY_UNKNOWN = 0,
    INTEGRITY_VERIFIED,
    INTEGRITY_FAILED,
    INTEGRITY_UNAVAILABLE
};

enum verity_state {
    VERITY_UNKNOWN = 0,
    VERITY_ENFORCED,
    VERITY_FAILED,
    VERITY_UNAVAILABLE
};

enum firmware_trust_state {
    FIRMWARE_TRUST_UNKNOWN = 0,
    FIRMWARE_TRUST_VERIFIED,
    FIRMWARE_TRUST_DISABLED,
    FIRMWARE_TRUST_UNAVAILABLE
};

enum tpm_state {
    TPM_STATE_UNKNOWN = 0,
    TPM_STATE_READY,
    TPM_STATE_FAILED,
    TPM_STATE_UNAVAILABLE
};

static int g_boot_degraded = 0;
static const char *g_boot_stage = "EARLY";
static struct nova_service g_services[MAX_SERVICES];
static int g_service_count = 0;
static unsigned long long g_app_catalog_generation = 0ULL;
static unsigned long long g_app_policy_generation = 0ULL;
static int g_app_security_listener=-1;
static int g_resource_security_listener=-1;
static enum trust_state g_trust = TRUST_UNKNOWN;
static enum integrity_state g_integrity = INTEGRITY_UNKNOWN;
static enum verity_state g_verity = VERITY_UNKNOWN;
static enum firmware_trust_state g_firmware_trust = FIRMWARE_TRUST_UNKNOWN;
static enum tpm_state g_tpm_state = TPM_STATE_UNKNOWN;
static int g_firmware_uefi = 0;
static int g_firmware_secure_boot = 0;
static int g_firmware_setup_mode = -1;
static int g_efivarfs_active = 0;
static unsigned long g_integrity_total = 0;
static unsigned long g_integrity_verified = 0;
static unsigned long g_integrity_failed = 0;
static int g_system_readonly = 0;
static int g_recovery_readonly = 0;
static int g_data_writable = 0;
static int g_storage_persistent = 0;
static int g_storage_ready = 0;
static char g_active_slot = 'A';
static char g_next_slot = 'A';
static char g_previous_slot = 'A';
static unsigned long g_slot_generation = 0;
static int g_slot_confirmed = 1;
static unsigned long g_boot_attempts_left = 0;
static int g_rollback_performed = 0;
static int g_boot_attempt_consumed = 0;
static int g_slot_metadata_valid = 1;
static int g_slot_metadata_recovered = 0;
static int g_slot_journal_write_failed = 0;
static unsigned long g_slot_metadata_seq = 0;
static char g_slot_metadata_source = '-';
static char g_pending_kind[NOVA_PENDING_KIND_MAX] = "none";
static char g_update_id[NOVA_UPDATE_ID_MAX] = "none";
static int g_update_bundle_verified = 0;
static char g_update_bundle_id[NOVA_UPDATE_ID_MAX] = "none";
static char g_update_bundle_version[NOVA_UPDATE_VERSION_MAX] = "unknown";
static char g_update_payload_sha256[65] = "";
static int g_verity_descriptor_verified = 0;
static enum verity_state g_recovery_verity = VERITY_UNKNOWN;
static int g_recovery_verity_descriptor_verified = 0;
static int g_recovery_verity_mapping_active = 0;
static char g_recovery_verity_root_hash[65] = "";
static char g_recovery_verity_salt[65] = "";
static char g_recovery_descriptor_source[24] = "unavailable";
static int g_verity_mapping_active = 0;
static int g_verity_rollback_scheduled = 0;
static char g_verity_root_hash[65] = "";
static char g_verity_salt[65] = "";
static long g_trust_key = -1;
static const char *g_system_device = "initramfs";
/* Resolved together, never fallback to a guessed block node. */
static char g_nova_storage_paths[4][32];
static unsigned long g_tpm_rollback_counter = 0;
static int g_tpm_counter_created = 0;
static int g_tpm_counter_commit_recovered = 0;
static unsigned long g_tpm_trust_epoch_counter = 0;
static int g_tpm_trust_counter_created = 0;
static int g_tpm_trust_epoch_commit_recovered = 0;
static int g_trust_epoch_rejected = 0;
static unsigned long g_active_rollback_index = 0;
static int g_antirollback_rejected = 0;
static int g_measured_boot = 0;
static int g_measurement_failed = 0;
static char g_pcr7_sha256[65] = "";
static char g_pcr14_before[65] = "";
static char g_pcr14_after[65] = "";
static char g_pcr7_source[16] = "none";
static char g_pcr14_before_source[16] = "none";
static char g_pcr14_after_source[16] = "none";
static char g_measurement_digest[65] = "";
static int g_event_log_verified = 0;
static char g_event_log_sha256[65] = "";
static char g_event_log_replayed_pcr7[65] = "";
static unsigned long g_event_log_bytes = 0;
static unsigned long g_event_log_events = 0;
static unsigned long g_event_log_pcr7_events = 0;
static char g_event_log_error[48] = "not-run";
static long g_release_key = -1;
static long g_release_next_key = -1;
static int g_release_policy_verified = 0;
static unsigned long g_release_trust_epoch = 0;
static char g_release_active_key_id[64] = "none";
static char g_release_active_cert_sha256[65] = "";
static char g_release_next_key_id[64] = "none";
static char g_release_next_cert_sha256[65] = "";
static char g_release_revoked_key_ids[256] = "none";
static int g_release_policy_format = 0;
static char g_release_policy_state[24] = "unavailable";
static char g_release_policy_sha256[65] = "";
static int g_preboot_release_policy_verified = 0;
static char g_preboot_release_policy_sha256[65] = "";
static int g_active_slot_descriptor_format = 0;
static char g_active_slot_descriptor_kind[32] = "unknown";
static char g_evidence_error[64] = "not-run";

static void write_boot_state(void);
static int execute_command(char *line);
static int g_user_workspace_ready;
static int normal_boot_policy_satisfied(void);
static int recovery_payload_verified(void);
static int ensure_pinned_root_key(const char *cert_path);
static int verify_signature_with_key(long key_id, const char *manifest_path, const char *signature_path);
static int verify_signature_with_loaded_root(const char *manifest_path, const char *signature_path);

static void copy_string(char *dst, nova_size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    nova_size_t i = 0;
    while (src && src[i] && i + 1 < cap) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static void ensure_dir(const char *path) { (void)nova_mkdir(path, 0755); }

static int string_has_prefix(const char *s, const char *prefix) {
    if(!s || !prefix) return 0;
    nova_size_t i=0;
    while(prefix[i]){ if(s[i]!=prefix[i]) return 0; i++; }
    return 1;
}

static int append_checked(char *dst, nova_size_t cap, const char *suffix) {
    if(!dst || !cap || !suffix) return 0;
    nova_size_t n=nova_strlen(dst), i=0;
    if(n>=cap) return 0;
    while(suffix[i]){
        if(n+1>=cap) return 0;
        dst[n++]=suffix[i++];
    }
    dst[n]='\0';
    return 1;
}

static int sandbox_app_data_path(const struct nova_service *svc, char *out, nova_size_t cap) {
    if(!svc || !svc->is_application || !svc->app_id[0] || !out || !cap) return 0;
    copy_string(out,cap,NOVA_APP_DATA_ROOT);
    if(!append_checked(out,cap,"/") || !append_checked(out,cap,svc->app_id)) return 0;
    return 1;
}

static unsigned long uptime_seconds(void) {
    struct nova_sysinfo s; nova_zero(&s, sizeof(s));
    return nova_sysinfo(&s) < 0 ? 0UL : (unsigned long)s.uptime;
}

static void log_prefix_fd(int fd, const char *level, const char *component, const char *event) {
    nova_fd_puts(fd, "ts="); nova_fd_put_u64(fd, uptime_seconds());
    nova_fd_puts(fd, " level="); nova_fd_puts(fd, level);
    nova_fd_puts(fd, " component="); nova_fd_puts(fd, component);
    nova_fd_puts(fd, " event="); nova_fd_puts(fd, event);
}

static void log_simple(const char *level, const char *component, const char *event, const char *message) {
    nova_puts("[novad]["); nova_puts(level); nova_puts("]["); nova_puts(component); nova_puts("] "); nova_putln(message);
    long fd = nova_open_append("/run/log/novad.log", 0644);
    if (fd >= 0) {
        log_prefix_fd((int)fd, level, component, event);
        nova_fd_puts((int)fd, " message="); nova_fd_putln((int)fd, message);
        nova_close((int)fd);
    }
}

static void log_service(const char *level, const char *event, const struct nova_service *svc, long status) {
    nova_puts("[novad]["); nova_puts(level); nova_puts("][services] "); nova_puts(event);
    nova_puts(" "); nova_puts(svc ? svc->name : "unknown");
    if (svc && svc->pid > 0) { nova_puts(" pid="); nova_put_u64((unsigned long)svc->pid); }
    if (status >= 0) { nova_puts(" status="); nova_put_u64((unsigned long)status); }
    nova_putln("");

    long fd = nova_open_append("/run/log/novad.log", 0644);
    if (fd >= 0) {
        log_prefix_fd((int)fd, level, "services", event);
        if (svc) {
            nova_fd_puts((int)fd, " name="); nova_fd_puts((int)fd, svc->name);
            nova_fd_puts((int)fd, " pid="); nova_fd_put_u64((int)fd, svc->pid > 0 ? (unsigned long)svc->pid : 0);
            nova_fd_puts((int)fd, " restarts="); nova_fd_put_u64((int)fd, svc->restart_count);
        }
        if (status >= 0) { nova_fd_puts((int)fd, " status="); nova_fd_put_u64((int)fd, (unsigned long)status); }
        nova_fd_putln((int)fd, "");
        nova_close((int)fd);
    }
}

static void log_integrity_failure(const char *path) {
    nova_puts("[novad][ERROR][integrity] mismatch "); nova_putln(path);
    long fd = nova_open_append("/run/log/novad.log", 0644);
    if (fd >= 0) {
        log_prefix_fd((int)fd, "ERROR", "integrity", "mismatch");
        nova_fd_puts((int)fd, " path="); nova_fd_putln((int)fd, path);
        nova_close((int)fd);
    }
}

static long read_text_file(const char *path, char *buffer, nova_size_t capacity) {
    if (!buffer || capacity < 2) return -1;
    long fd = nova_open_ro(path); if (fd < 0) return fd;
    nova_size_t used = 0;
    while (used + 1 < capacity) {
        long n = nova_read((int)fd, buffer + used, capacity - used - 1);
        if (n < 0) { nova_close((int)fd); return n; }
        if (n == 0) break;
        used += (nova_size_t)n;
    }
    if (used + 1 >= capacity) {
        char extra = 0;
        long more = nova_read((int)fd, &extra, 1);
        if (more > 0) { buffer[capacity - 1] = '\0'; nova_close((int)fd); return -2; }
    }
    buffer[used] = '\0'; nova_close((int)fd); return (long)used;
}

static long read_binary_file(const char *path, unsigned char *buffer, nova_size_t capacity) {
    if (!buffer || capacity == 0) return -1;
    long fd = nova_open_ro(path); if (fd < 0) return fd;
    nova_size_t used = 0;
    while (used < capacity) {
        long n = nova_read((int)fd, buffer + used, capacity - used);
        if (n < 0) { nova_close((int)fd); return n; }
        if (n == 0) break;
        used += (nova_size_t)n;
    }
    if (used == capacity) {
        unsigned char extra = 0;
        long more = nova_read((int)fd, &extra, 1);
        if (more > 0) { nova_close((int)fd); return -2; }
    }
    nova_close((int)fd);
    return (long)used;
}

/* Phase-19 bounded, fail-closed GPT PARTUUID verification. */
#include "phase19_storage_discovery.inc"

static const char *firmware_trust_name(void) {
    switch(g_firmware_trust){
        case FIRMWARE_TRUST_VERIFIED: return "VERIFIED";
        case FIRMWARE_TRUST_DISABLED: return "DISABLED";
        case FIRMWARE_TRUST_UNAVAILABLE: return "UNAVAILABLE";
        default: return "UNKNOWN";
    }
}

static const char *tpm_state_name(void) {
    switch(g_tpm_state){
        case TPM_STATE_READY: return "READY";
        case TPM_STATE_FAILED: return "FAILED";
        case TPM_STATE_UNAVAILABLE: return "UNAVAILABLE";
        default: return "UNKNOWN";
    }
}

static int hex_nibble(char c, unsigned char *v) {
    if(c>='0'&&c<='9'){*v=(unsigned char)(c-'0');return 1;}
    if(c>='a'&&c<='f'){*v=(unsigned char)(10+c-'a');return 1;}
    if(c>='A'&&c<='F'){*v=(unsigned char)(10+c-'A');return 1;}
    return 0;
}

static int hex32_to_bytes(const char *hex, unsigned char out[32]) {
    if(!hex || nova_strlen(hex)!=64) return 0;
    for(int i=0;i<32;i++){
        unsigned char hi=0,lo=0;
        if(!hex_nibble(hex[i*2],&hi)||!hex_nibble(hex[i*2+1],&lo)) return 0;
        out[i]=(unsigned char)((hi<<4)|lo);
    }
    return 1;
}

static void initialize_tpm_policy(void) {
    g_boot_stage="TPM_INIT";
    if(!nova_tpm2_present()){
        g_tpm_state=TPM_STATE_UNAVAILABLE; g_boot_degraded=1;
        log_simple("ERROR","tpm","unavailable","TPM 2.0 is unavailable; anti-rollback and measured boot cannot be enforced");
        return;
    }
    /* Do not create or mutate persistent TPM policy state from an untrusted
     * firmware boot.  Diagnostic boots may inspect that a TPM exists, but
     * first-use NV provisioning is allowed only after UEFI Secure Boot trust
     * has been established. */
    if(g_firmware_trust!=FIRMWARE_TRUST_VERIFIED){
        g_tpm_state=TPM_STATE_UNAVAILABLE; g_boot_degraded=1;
        log_simple("WARN","tpm","firmware_gate","TPM NV anti-rollback state is not provisioned or mutated without verified firmware trust");
        return;
    }
    unsigned long value=0, trust_value=0; int created=0, trust_created=0;
    if(!nova_tpm2_prepare_rollback_counter(&value,&created)){
        g_tpm_state=TPM_STATE_FAILED; g_boot_degraded=1;
        log_simple("ERROR","tpm","counter_prepare_failed","TPM rollback counter could not be opened or provisioned");
        return;
    }
    if(!nova_tpm2_prepare_trust_epoch_counter(&trust_value,&trust_created)){
        g_tpm_state=TPM_STATE_FAILED; g_boot_degraded=1;
        log_simple("ERROR","tpm","trust_counter_prepare_failed","TPM release-trust epoch counter could not be opened or provisioned");
        return;
    }
    g_tpm_state=TPM_STATE_READY; g_tpm_rollback_counter=value; g_tpm_counter_created=created;
    g_tpm_trust_epoch_counter=trust_value; g_tpm_trust_counter_created=trust_created;
    /* PCR measurement health is evaluated later, after signed System/Recovery
     * trust and integrity are established.  Do not conflate a PCR-read problem
     * with the independent NV anti-rollback counter state. */
    log_simple("INFO","tpm",created?"counter_created":"counter_ready",created?
               "TPM monotonic rollback counter provisioned for Stage-4C":"TPM monotonic rollback counter loaded");
    log_simple("INFO","tpm",trust_created?"trust_counter_created":"trust_counter_ready",trust_created?
               "TPM release-trust epoch counter provisioned for Stage-4D":"TPM release-trust epoch counter loaded");
}

static int read_efivar_u8(const char *path, int *value) {
    unsigned char data[16];
    long n=read_binary_file(path,data,sizeof(data));
    if(n<5) return 0;
    if(value) *value=(int)data[4];
    return 1;
}

static void detect_firmware_trust(void) {
    g_boot_stage="FIRMWARE_TRUST";
    char platform[32];
    if(read_text_file("/sys/firmware/efi/fw_platform_size",platform,sizeof(platform))<0){
        g_firmware_uefi=0; g_firmware_secure_boot=0; g_firmware_setup_mode=-1;
        g_firmware_trust=FIRMWARE_TRUST_UNAVAILABLE; g_boot_degraded=1;
        log_simple("ERROR","firmware","uefi_unavailable","UEFI runtime state is unavailable; Stage-4C refuses normal service startup");
        return;
    }
    g_firmware_uefi=1;
    ensure_dir(NOVA_EFIVARS_DIR);
    int secure=-1, setup=-1;
    if(nova_getpid()==1 && nova_mount("efivarfs",NOVA_EFIVARS_DIR,"efivarfs",NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC,(void *)0)>=0)
        g_efivarfs_active=1;
    if(!read_efivar_u8(NOVA_EFI_SECUREBOOT_VAR,&secure) || !read_efivar_u8(NOVA_EFI_SETUPMODE_VAR,&setup)){
        g_firmware_secure_boot=0; g_firmware_setup_mode=-1;
        g_firmware_trust=FIRMWARE_TRUST_UNAVAILABLE; g_boot_degraded=1;
        log_simple("ERROR","firmware","secure_boot_state_unavailable","UEFI detected but SecureBoot/SetupMode variables could not be read through efivarfs");
        return;
    }
    g_firmware_secure_boot=(secure==1);
    g_firmware_setup_mode=setup;
    if(secure==1 && setup==0){
        g_firmware_trust=FIRMWARE_TRUST_VERIFIED;
        log_simple("INFO","firmware","secure_boot_verified","UEFI Secure Boot is enabled and firmware trust policy is active");
    } else {
        g_firmware_trust=FIRMWARE_TRUST_DISABLED; g_boot_degraded=1;
        log_simple("ERROR","firmware","secure_boot_disabled","UEFI boot was not protected by active Secure Boot policy");
    }
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + nova_strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) end--;
    *end = '\0'; return s;
}

static void try_mount(const char *src, const char *target, const char *type, unsigned long flags, const void *data) {
    if (nova_mount(src, target, type, flags, data) < 0) {
        g_boot_degraded = 1;
        nova_puts("[novad][WARN][mount] failed "); nova_putln(target);
    }
}

static int probe_writable(const char *path) {
    long fd = nova_open_create_excl(path, 0600);
    if (fd < 0) return 0;
    nova_fd_puts((int)fd, "probe\n"); nova_close((int)fd); (void)nova_unlink(path); return 1;
}

static int seal_readonly_tree(const char *path) {
    if (nova_getpid() != 1) return 0;
    if (nova_mount(path, path, (const char *)0, NOVA_MS_BIND, (void *)0) < 0) return 0;
    if (nova_mount((const char *)0, path, (const char *)0,
                   NOVA_MS_REMOUNT | NOVA_MS_BIND | NOVA_MS_RDONLY, (void *)0) < 0) return 0;
    char probe[96]; copy_string(probe, sizeof(probe), path);
    nova_size_t n = nova_strlen(probe); const char suffix[] = "/.nova-ro-probe";
    for (nova_size_t i=0; suffix[i] && n+1<sizeof(probe); i++) probe[n++] = suffix[i];
    probe[n]='\0';
    long fd = nova_open_create_excl(probe, 0600);
    if (fd >= 0) { nova_close((int)fd); (void)nova_unlink(probe); return 0; }
    return 1;
}

static void bootstrap_filesystems(void) {
    g_boot_stage = "FILESYSTEMS";
    if (nova_getpid() != 1) return;
    ensure_dir("/proc"); ensure_dir("/sys"); ensure_dir("/dev"); ensure_dir("/run"); ensure_dir("/tmp");
    ensure_dir("/System"); ensure_dir("/Data"); ensure_dir("/Recovery");
    try_mount("proc", "/proc", "proc", 0, (void *)0);
    try_mount("sysfs", "/sys", "sysfs", 0, (void *)0);
    ensure_dir("/sys/kernel"); ensure_dir("/sys/kernel/security");
    try_mount("securityfs", "/sys/kernel/security", "securityfs", NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC, (void *)0);
    (void)nova_mount("devtmpfs", "/dev", "devtmpfs", 0, (void *)0);
    try_mount("tmpfs", "/run", "tmpfs", 0, (void *)0);
    try_mount("tmpfs", "/tmp", "tmpfs", 0, (void *)0);
    ensure_dir("/run/log"); ensure_dir("/run/nova");
}

static unsigned long parse_unsigned(const char *s, int *ok) {
    unsigned long value=0; int seen=0;
    const unsigned long max_value=~0UL;
    while(s && *s>='0' && *s<='9'){
        unsigned long digit=(unsigned long)(*s-'0');
        if(value>(max_value-digit)/10UL){if(ok)*ok=0;return 0;}
        value=value*10UL+digit; s++; seen=1;
    }
    if(ok)*ok=seen && (!s || *s=='\0');
    return value;
}

static int checked_add_ul(unsigned long base, unsigned long delta, unsigned long *out) {
    if(!out || delta>(~0UL)-base) return 0;
    *out=base+delta;
    return 1;
}

static int parse_unsigned_canonical(const char *s, unsigned long *out) {
    int ok=0;
    if(!s || !*s || (s[0]=='0' && s[1]!='\0')) return 0;
    unsigned long value=parse_unsigned(s,&ok);
    if(!ok) return 0;
    if(out) *out=value;
    return 1;
}


static void sha_update_string(struct nova_sha256_ctx *ctx, const char *s) {
    if (s) nova_sha256_update(ctx, s, nova_strlen(s));
}

static void sha_update_u64_ascii(struct nova_sha256_ctx *ctx, unsigned long value) {
    char buf[32]; nova_size_t i=0;
    if(value==0){ char z='0'; nova_sha256_update(ctx,&z,1); return; }
    while(value && i<sizeof(buf)){buf[i++]=(char)('0'+(value%10));value/=10;}
    while(i){char c=buf[--i];nova_sha256_update(ctx,&c,1);}
}

static int safe_hex(const char *s, nova_size_t exact_len);

static int safe_metadata_token(const char *s, nova_size_t max_len) {
    if(!s || !*s) return 0;
    nova_size_t n=0;
    while(s[n]){
        char c=s[n];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')) return 0;
        n++; if(n>=max_len) return 0;
    }
    return 1;
}

static int safe_display_name(const char *s, nova_size_t max_len) {
    if(!s || !*s) return 0;
    nova_size_t n=0;
    while(s[n]){
        unsigned char c=(unsigned char)s[n];
        if(c<0x20U || c>0x7eU || c=='/' || c=='\\') return 0;
        n++; if(n>=max_len) return 0;
    }
    return 1;
}

static int safe_app_description(const char *s, nova_size_t max_len) {
    if(!s || !*s) return 0;
    nova_size_t n=0;
    while(s[n]){
        unsigned char c=(unsigned char)s[n];
        if(c<0x20U || c>0x7eU) return 0;
        n++; if(n>=max_len) return 0;
    }
    return 1;
}

static int application_category_from_name(const char *name, unsigned int *out) {
    unsigned int value=NOVA_APP_CATEGORY_UNSPECIFIED;
    if(!name || !*name || !out) return 0;
    if(nova_streq(name,"unspecified")) value=NOVA_APP_CATEGORY_UNSPECIFIED;
    else if(nova_streq(name,"system")) value=NOVA_APP_CATEGORY_SYSTEM;
    else if(nova_streq(name,"productivity")) value=NOVA_APP_CATEGORY_PRODUCTIVITY;
    else if(nova_streq(name,"development")) value=NOVA_APP_CATEGORY_DEVELOPMENT;
    else if(nova_streq(name,"media")) value=NOVA_APP_CATEGORY_MEDIA;
    else if(nova_streq(name,"communication")) value=NOVA_APP_CATEGORY_COMMUNICATION;
    else if(nova_streq(name,"game")) value=NOVA_APP_CATEGORY_GAME;
    else if(nova_streq(name,"utility")) value=NOVA_APP_CATEGORY_UTILITY;
    else return 0;
    *out=value;
    return 1;
}

static int application_capability_token(const char *token, unsigned long long *bit, unsigned int *rank) {
    if(!token || !*token || !bit || !rank) return 0;
    if(nova_streq(token,NOVA_APP_CAP_TOKEN_UI_SURFACE)){ *bit=NOVA_APP_CAP_UI_SURFACE; *rank=1U; return 1; }
    if(nova_streq(token,NOVA_APP_CAP_TOKEN_INPUT_TEXT)){ *bit=NOVA_APP_CAP_INPUT_TEXT; *rank=2U; return 1; }
    if(nova_streq(token,NOVA_APP_CAP_TOKEN_INPUT_TAB_CAPTURE)){ *bit=NOVA_APP_CAP_INPUT_TAB_CAPTURE; *rank=3U; return 1; }
    if(nova_streq(token,NOVA_APP_CAP_TOKEN_CLIPBOARD_READ)){ *bit=NOVA_APP_CAP_CLIPBOARD_READ; *rank=4U; return 1; }
    if(nova_streq(token,NOVA_APP_CAP_TOKEN_CLIPBOARD_WRITE)){ *bit=NOVA_APP_CAP_CLIPBOARD_WRITE; *rank=5U; return 1; }
    if(nova_streq(token,NOVA_APP_CAP_TOKEN_RESOURCE_DEMO_READ)){ *bit=NOVA_APP_CAP_RESOURCE_DEMO_READ; *rank=6U; return 1; }
    if(nova_streq(token,NOVA_APP_CAP_TOKEN_RESOURCE_NOTES_READ)){ *bit=NOVA_APP_CAP_RESOURCE_NOTES_READ; *rank=7U; return 1; }
    return 0;
}

static int parse_application_capabilities(const char *value, unsigned long long *out) {
    if(!value || !*value || !out) return 0;
    if(nova_streq(value,"none")){ *out=0ULL; return 1; }
    unsigned long long mask=0ULL;
    unsigned int previous_rank=0U;
    const char *p=value;
    while(*p){
        char token[32];
        unsigned long n=0UL;
        while(*p && *p!=','){
            if(n+1UL>=sizeof(token)) return 0;
            token[n++]=*p++;
        }
        if(n==0UL) return 0;
        token[n]='\0';
        if(*p==','){ p++; if(!*p) return 0; }
        if(!safe_metadata_token(token,sizeof(token))) return 0;
        unsigned long long bit=0ULL;
        unsigned int rank=0U;
        if(!application_capability_token(token,&bit,&rank)) return 0;
        if(rank<=previous_rank || (mask&bit)!=0ULL) return 0;
        mask|=bit;
        previous_rank=rank;
    }
    if((mask&~((NOVA_APP_CAP_PHASE16_KNOWN_MASK|NOVA_APP_CAP_RESOURCE_DEMO_READ)|NOVA_APP_CAP_RESOURCE_NOTES_READ))!=0ULL) return 0;
    *out=mask;
    return 1;
}

static unsigned long long application_policy_effective_capabilities(unsigned long long declared) {
    if((declared&~NOVA_APP_CAP_PHASE18_STEP5_KNOWN_MASK)!=0ULL) return 0ULL;
    /* Phase-18 adds one genuinely broker-enforced resource; all permissions
     * remain strictly declared and later policy layers may only reduce them. */
    return declared&NOVA_APP_CAP_PHASE18_STEP5_KNOWN_MASK;
}

static int application_capability_policy_valid(const struct nova_service *svc) {
    if(!svc || !svc->is_application) return 0;
    if((svc->declared_capabilities&~((NOVA_APP_CAP_PHASE16_KNOWN_MASK|NOVA_APP_CAP_RESOURCE_DEMO_READ)|NOVA_APP_CAP_RESOURCE_NOTES_READ))!=0ULL) return 0;
    if((svc->effective_capabilities&~((NOVA_APP_CAP_PHASE16_KNOWN_MASK|NOVA_APP_CAP_RESOURCE_DEMO_READ)|NOVA_APP_CAP_RESOURCE_NOTES_READ))!=0ULL) return 0;
    if((svc->effective_capabilities&~svc->declared_capabilities)!=0ULL) return 0;
    return 1;
}

static int application_sandbox_profile_from_name(const char *name, unsigned int *profile,
                                                 unsigned long long *required_flags) {
    if(!name || !*name || !profile || !required_flags) return 0;
#if defined(NOVA_R47_BROWSER_COMPAT) && NOVA_R47_BROWSER_COMPAT
    if(nova_streq(name,NOVA_APP_SANDBOX_PROFILE_BROWSER_V1_NAME)){
        *profile=NOVA_APP_SANDBOX_PROFILE_BROWSER_V1;*required_flags=NOVA_APP_SANDBOX_BROWSER_V1_FLAGS;return 1;
    }
#endif
    if(nova_streq(name,NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1_NAME)){
        *profile=NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1;
        *required_flags=NOVA_APP_SANDBOX_DESKTOP_V1_FLAGS;
        return 1;
    }
    if(nova_streq(name,NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1_NAME)){
        *profile=NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1;
        *required_flags=NOVA_APP_SANDBOX_FILES_READ_V1_FLAGS;return 1;
    }
    return 0;
}

static int application_sandbox_policy_valid(const struct nova_service *svc) {
    if(!svc || !svc->is_application) return 0;
    if(svc->files_user_home!=0&&svc->files_user_home!=1)return 0;
#if defined(NOVA_R47_BROWSER_COMPAT) && NOVA_R47_BROWSER_COMPAT
    if(svc->sandbox_profile==NOVA_APP_SANDBOX_PROFILE_BROWSER_V1){
        return !svc->files_user_home && nova_streq(svc->app_id,"nova.browser") && svc->client_id==4U &&
         nova_streq(svc->exec,"/System/Apps/nova-browser-session") &&
         svc->declared_capabilities==(NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT|NOVA_APP_CAP_INPUT_TAB_CAPTURE) &&
         svc->sandbox_required_flags==NOVA_APP_SANDBOX_BROWSER_V1_FLAGS;
    }
#endif
    if(nova_streq(svc->app_id,"nova.browser")||nova_streq(svc->exec,"/System/Apps/nova-browser-session"))return 0;

    if(svc->files_user_home&&svc->sandbox_profile!=NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1)return 0;
    if(svc->sandbox_profile==NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1){
        return nova_streq(svc->app_id,"nova.files") && svc->client_id==3U &&
            nova_streq(svc->exec,"/System/Apps/nova-files") &&
            svc->declared_capabilities==(NOVA_APP_CAP_UI_SURFACE|NOVA_APP_CAP_INPUT_TEXT) &&
            svc->sandbox_required_flags==NOVA_APP_SANDBOX_FILES_READ_V1_FLAGS;
    }
    if(nova_streq(svc->app_id,"nova.files")||
       nova_streq(svc->exec,"/System/Apps/nova-files"))return 0;
    if(svc->sandbox_profile!=NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1) return 0;
    if(svc->sandbox_required_flags!=NOVA_APP_SANDBOX_DESKTOP_V1_FLAGS) return 0;
    if((svc->sandbox_required_flags&~NOVA_APP_SANDBOX_KNOWN_FLAGS)!=0ULL) return 0;
    return 1;
}

static long sandbox_open_files_source(const char *path) {
    struct {unsigned long long flags,mode,resolve;} how={
        NOVA_O_RDONLY|0200000|NOVA_O_CLOEXEC|NOVA_O_NOFOLLOW,0,0x06};
    return nova_syscall4(NOVA_FILES_NR_OPENAT2,NOVA_AT_FDCWD,(long)path,(long)&how,sizeof(how));
}
static int sandbox_files_parent_trusted(void) {
    long fd=sandbox_open_files_source(NOVA_APP_DATA_ROOT);if(fd<0)return 0;
    struct nova_statx st;nova_zero(&st,sizeof(st));
    long rc=nova_syscall5(NOVA_NR_statx,fd,(long)"",NOVA_AT_EMPTY_PATH,0x7ff,(long)&st);
    nova_close((int)fd);
    return rc>=0 && st.stx_uid==0U && !(st.stx_mode&0022U);
}

static int sandbox_prepare_persistent_data(const struct nova_service *svc) {
    char app_path[96];
    if(!application_sandbox_policy_valid(svc) || !sandbox_app_data_path(svc,app_path,sizeof(app_path))) return 0;
    /* Browser profile/home is session-only inside its own bounded tmpfs. */
    if(svc->sandbox_profile==NOVA_APP_SANDBOX_PROFILE_BROWSER_V1)return 1;
    ensure_dir(NOVA_APP_DATA_ROOT);
    if(svc->sandbox_profile==NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1){
        if(!sandbox_files_parent_trusted())return 0;
        (void)nova_mkdir(app_path,0700);
        long fd=sandbox_open_files_source(app_path);if(fd<0)return 0;
        long own=nova_syscall5(NOVA_NR_fchownat,fd,(long)"",3,3,NOVA_AT_EMPTY_PATH);
#if defined(__x86_64__)
        long mode=nova_syscall2(91,fd,0700); /* fchmod */
#else
        long mode=nova_syscall2(52,fd,0700);
#endif
        nova_close((int)fd);return own>=0&&mode>=0;
    }
    (void)nova_mkdir(app_path,0700);
    if(nova_fchownat(app_path,svc->client_id,svc->client_id)<0) return 0;
    if(nova_fchmodat(app_path,0700)<0) return 0;
    return 1;
}

static int sandbox_create_dir(const char *path, unsigned long mode) {
    (void)nova_mkdir(path,mode);
    return nova_fchmodat(path,mode)>=0;
}

static int sandbox_bind_mount(const char *source, const char *target, unsigned long remount_flags) {
    if(nova_mount(source,target,(const char *)0,NOVA_MS_BIND,(void *)0)<0) return 0;
    if(nova_mount((const char *)0,target,(const char *)0,NOVA_MS_REMOUNT|NOVA_MS_BIND|remount_flags,(void *)0)<0) return 0;
    return 1;
}

static int sandbox_build_private_root(const struct nova_service *svc) {
    char app_data[96];
    char exec_target[256];
    char path[256];
    long fd=-1;
    if(!sandbox_app_data_path(svc,app_data,sizeof(app_data))) return 0;
    if(!svc->exec[0] || !string_has_prefix(svc->exec,"/System/Apps/")) return 0;

    /* Each application has its own mount namespace, so the fixed staging path
     * is safe: the tmpfs mount below is private to this child. */
    ensure_dir(NOVA_APP_SANDBOX_ROOT);
    if(nova_mount("tmpfs",NOVA_APP_SANDBOX_ROOT,"tmpfs",NOVA_MS_NOSUID|NOVA_MS_NODEV,"mode=0755,size=4m")<0) return 0;

    copy_string(path,sizeof(path),NOVA_APP_SANDBOX_ROOT); if(!append_checked(path,sizeof(path),"/System")) return 0;
    if(!sandbox_create_dir(path,0755)) return 0;
    if(!append_checked(path,sizeof(path),"/Apps")) return 0;
    if(!sandbox_create_dir(path,0755)) return 0;

    copy_string(exec_target,sizeof(exec_target),NOVA_APP_SANDBOX_ROOT);
    if(!append_checked(exec_target,sizeof(exec_target),svc->exec)) return 0;
    fd=nova_open_write(exec_target,0555); if(fd<0) return 0; nova_close((int)fd);
    if(!sandbox_bind_mount(svc->exec,exec_target,NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV)) return 0;

    copy_string(path,sizeof(path),NOVA_APP_SANDBOX_ROOT); if(!append_checked(path,sizeof(path),"/Data")) return 0;
    if(!sandbox_create_dir(path,0755)) return 0;
    if(!append_checked(path,sizeof(path),"/App")) return 0;
    if(!sandbox_create_dir(path,0700)) return 0;
    if(svc->sandbox_profile==NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1){
        if(!sandbox_files_parent_trusted())return 0;
        if(svc->files_user_home&&!g_user_workspace_ready)return 0;
        const char *files_source=svc->files_user_home?"/run/nova/user-session/home":app_data;
        long source=sandbox_open_files_source(files_source);if(source<0)return 0;nova_close((int)source);
        if(!sandbox_bind_mount(files_source,path,NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC))return 0;
    }else if(!sandbox_bind_mount(app_data,path,NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC)) return 0;

    copy_string(path,sizeof(path),NOVA_APP_SANDBOX_ROOT); if(!append_checked(path,sizeof(path),"/tmp")) return 0;
    if(!sandbox_create_dir(path,0755)) return 0;
    if(nova_mount("tmpfs",path,"tmpfs",NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC,"mode=1777,size=16m")<0) return 0;

    copy_string(path,sizeof(path),NOVA_APP_SANDBOX_ROOT); if(!append_checked(path,sizeof(path),"/run")) return 0;
    if(!sandbox_create_dir(path,0755)) return 0;
    if(!append_checked(path,sizeof(path),"/nova")) return 0;
    if(!sandbox_create_dir(path,0755)) return 0;
    if((svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_PRIVATE_RUNTIME_IPC)!=0ULL){
        /* Bind the dedicated app-facing runtime directory, not /run/nova itself.
         * Phase-17 introduced client.sock here; Phase-18 may add authenticated
         * app-facing broker endpoints without exposing novad control sockets. */
        if(!sandbox_bind_mount(NOVA_SURFACE_SANDBOX_SOCKET_DIR,path,NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC)) return 0;
    }

    if(nova_chroot(NOVA_APP_SANDBOX_ROOT)<0) return 0;
    if(nova_chdir("/")<0) return 0;
    return 1;
}

static int sandbox_drop_application_privileges(const struct nova_service *svc) {
    if(!svc || !svc->client_id) return 0;
    if(nova_setgroups(0,(const unsigned int *)0)<0) return 0;
    if(nova_setgid(svc->client_id)<0) return 0;
    if(nova_setuid(svc->client_id)<0) return 0;
    return 1;
}

static int sandbox_install_desktop_seccomp(void) {
    static struct nova_sock_filter filter[] = {
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARCH_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_AUDIT_ARCH_NATIVE,1,0),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_KILL_PROCESS),

        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_NR_OFFSET),

        /* Socket creation is restricted to the compositor transport shape.
         * The private runtime mount further restricts the reachable pathname to
         * /run/nova/client.sock. */
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_NR_socket,0,11),
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARG0_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_AF_UNIX,1,0),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARG1_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_SOCK_SEQPACKET,2,0),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,1,0),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARG2_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,0U,1,0),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ALLOW),

#define NOVA_SECCOMP_ALLOW_NR(nr_) \
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,(nr_),0,1), \
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ALLOW)
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_read),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_write),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_close),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_lseek),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_mmap),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_munmap),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_nanosleep),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_clock_gettime),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_getpid),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_connect),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_sendmsg),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_recvmsg),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_memfd_create),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_ftruncate),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_openat),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_ppoll),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_fsync),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_mkdirat),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_unlinkat),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_renameat),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_fchmodat),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_getcwd),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_execve),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_exit),
#undef NOVA_SECCOMP_ALLOW_NR

        /* Unknown or process/privilege-management syscalls fail closed. */
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
    };
    struct nova_sock_fprog program;
    program.len=(unsigned short)(sizeof(filter)/sizeof(filter[0]));
    program.filter=filter;
    return nova_prctl(NOVA_PR_SET_SECCOMP,NOVA_SECCOMP_MODE_FILTER,(unsigned long)&program,0UL,0UL)>=0;
}

static int sandbox_install_files_seccomp(void) {
    static struct nova_sock_filter filter[] = {
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARCH_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_AUDIT_ARCH_NATIVE,1,0),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_KILL_PROCESS),

        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_NR_OFFSET),

        /* Socket creation is restricted to the compositor transport shape.
         * The private runtime mount further restricts the reachable pathname to
         * /run/nova/client.sock. */
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_NR_socket,0,11),
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARG0_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_AF_UNIX,1,0),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARG1_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_SOCK_SEQPACKET,2,0),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,1,0),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARG2_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,0U,1,0),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ALLOW),

#define NOVA_SECCOMP_ALLOW_NR(nr_) \
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,(nr_),0,1), \
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ALLOW)
        NOVA_SECCOMP_ALLOW_NR(NOVA_FILES_NR_GETDENTS64),
        NOVA_SECCOMP_ALLOW_NR(NOVA_FILES_NR_OPENAT2),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,NOVA_FILES_NR_FCNTL,0,4),
        NOVA_BPF_STMT(NOVA_BPF_LD|NOVA_BPF_W|NOVA_BPF_ABS,NOVA_SECCOMP_DATA_ARG1_OFFSET),
        NOVA_BPF_JUMP(NOVA_BPF_JMP|NOVA_BPF_JEQ|NOVA_BPF_K,3U,1,0), /* F_GETFL only */
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ALLOW),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_statx),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_getsockopt),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_read),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_write),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_close),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_lseek),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_mmap),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_munmap),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_nanosleep),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_clock_gettime),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_getpid),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_connect),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_sendmsg),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_recvmsg),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_memfd_create),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_ftruncate),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_ppoll),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_execve),
        NOVA_SECCOMP_ALLOW_NR(NOVA_NR_exit),
#undef NOVA_SECCOMP_ALLOW_NR

        /* Unknown or process/privilege-management syscalls fail closed. */
        NOVA_BPF_STMT(NOVA_BPF_RET|NOVA_BPF_K,NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM),
    };
    struct nova_sock_fprog program;
    program.len=(unsigned short)(sizeof(filter)/sizeof(filter[0]));
    program.filter=filter;
    return nova_prctl(NOVA_PR_SET_SECCOMP,NOVA_SECCOMP_MODE_FILTER,(unsigned long)&program,0UL,0UL)>=0;
}

#include "nova_home_directory_worker.inc"

static int sandbox_delegate_files_directory(void) {
    /* Called only after private root and inherited FD closure, before uid drop. */
    int pair[2]={-1,-1};int root=-1;int ok=0;
    long fd=nova_syscall4(NOVA_NR_openat,NOVA_AT_FDCWD,(long)"/Data/App",
                         NOVA_O_RDONLY|0200000|NOVA_O_NOFOLLOW|NOVA_O_CLOEXEC,0);
    if(fd<0)return 0;root=(int)fd;
    if(nova_syscall4(NOVA_FILES_NR_SOCKETPAIR,NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET,0,(long)pair)<0)goto done;
    if(!nova_files_send_grant(pair[0],root))goto done;
    /* Root is 3 after close_range. Close it, then place the RX endpoint at 3. */
    nova_close(root);root=-1;
    if(nova_syscall3(NOVA_FILES_NR_DUP3,pair[1],NOVA_FILES_GRANT_SOCKET_FD,0)<0)goto done;
    ok=1;
 done:
    if(root>=0)nova_close(root);
    if(pair[0]>=0)nova_close(pair[0]);
    if(pair[1]>=0)nova_close(pair[1]);
    if(!ok)nova_close(NOVA_FILES_GRANT_SOCKET_FD);
    return ok;
}

static int apply_application_sandbox(const struct nova_service *svc) {
    if(!application_sandbox_policy_valid(svc)) return 0;
    if((svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_MOUNT_NAMESPACE)!=0ULL){
        if(nova_unshare(NOVA_CLONE_NEWNS)<0) return 0;
    }
    if((svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_PRIVATE_MOUNTS)!=0ULL){
        if(nova_mount((const char *)0,"/",(const char *)0,NOVA_MS_REC|NOVA_MS_PRIVATE,(void *)0)<0) return 0;
    }
    if((svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_PRIVATE_ROOT)!=0ULL ||
       (svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_PRIVATE_TMP)!=0ULL ||
       (svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_APP_DATA)!=0ULL){
        if(!sandbox_build_private_root(svc)) return 0;
    }
    if((svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_NO_NEW_PRIVS)!=0ULL){
        if(nova_prctl(NOVA_PR_SET_NO_NEW_PRIVS,1UL,0UL,0UL,0UL)<0) return 0;
    }
    if((svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_CLOSE_INHERITED_FDS)!=0ULL){
        if(nova_close_range(3U,0xffffffffU,0U)<0) return 0;
    }
    if(svc->sandbox_profile==NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1){
        if(svc->files_user_home){if(!g_user_workspace_ready||!delegate_home_directory())return 0;}
        else if(!sandbox_delegate_files_directory())return 0;
    }
    if((svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_DROP_PRIVILEGES)!=0ULL){
        if(!sandbox_drop_application_privileges(svc)) return 0;
    }
    if((svc->sandbox_required_flags&NOVA_APP_SANDBOX_FLAG_SECCOMP_DESKTOP_V1)!=0ULL){
        if(svc->sandbox_profile==NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1){
            if(!sandbox_install_files_seccomp())return 0;
        }else if(!sandbox_install_desktop_seccomp()) return 0;
    }
    return 1;
}

struct nova_slot_record {
    unsigned long record_seq;
    char active;
    char previous;
    unsigned long generation;
    int confirmed;
    unsigned long attempts_left;
    char pending_kind[NOVA_PENDING_KIND_MAX];
    char update_id[NOVA_UPDATE_ID_MAX];
};

static void slot_metadata_checksum_v3(unsigned long record_seq, char active, char previous,
                                      unsigned long generation, int confirmed,
                                      unsigned long attempts_left, const char *pending_kind,
                                      const char *update_id, char out[65]) {
    struct nova_sha256_ctx ctx; unsigned char digest[32];
    nova_sha256_init(&ctx);
    sha_update_string(&ctx,"format=3\nrecord_seq="); sha_update_u64_ascii(&ctx,record_seq);
    sha_update_string(&ctx,"\nactive="); nova_sha256_update(&ctx,&active,1);
    sha_update_string(&ctx,"\nprevious="); nova_sha256_update(&ctx,&previous,1);
    sha_update_string(&ctx,"\ngeneration="); sha_update_u64_ascii(&ctx,generation);
    sha_update_string(&ctx,"\nconfirmed="); sha_update_string(&ctx,confirmed?"yes":"no");
    sha_update_string(&ctx,"\nattempts_left="); sha_update_u64_ascii(&ctx,attempts_left);
    sha_update_string(&ctx,"\npending_kind="); sha_update_string(&ctx,pending_kind);
    sha_update_string(&ctx,"\nupdate_id="); sha_update_string(&ctx,update_id);
    sha_update_string(&ctx,"\n");
    nova_sha256_final(&ctx,digest); nova_sha256_hex(digest,out);
}

static void slot_metadata_checksum_v2(char active, char previous, unsigned long generation, int confirmed,
                                      unsigned long attempts_left, const char *pending_kind,
                                      const char *update_id, char out[65]) {
    struct nova_sha256_ctx ctx; unsigned char digest[32];
    nova_sha256_init(&ctx);
    sha_update_string(&ctx,"format=2\nactive="); nova_sha256_update(&ctx,&active,1);
    sha_update_string(&ctx,"\nprevious="); nova_sha256_update(&ctx,&previous,1);
    sha_update_string(&ctx,"\ngeneration="); sha_update_u64_ascii(&ctx,generation);
    sha_update_string(&ctx,"\nconfirmed="); sha_update_string(&ctx,confirmed?"yes":"no");
    sha_update_string(&ctx,"\nattempts_left="); sha_update_u64_ascii(&ctx,attempts_left);
    sha_update_string(&ctx,"\npending_kind="); sha_update_string(&ctx,pending_kind);
    sha_update_string(&ctx,"\nupdate_id="); sha_update_string(&ctx,update_id);
    sha_update_string(&ctx,"\n");
    nova_sha256_final(&ctx,digest); nova_sha256_hex(digest,out);
}

static int write_slot_record(const char *path, const char *tmp, unsigned long record_seq,
                             char active, char previous, unsigned long generation, int confirmed,
                             unsigned long attempts_left, const char *pending_kind,
                             const char *update_id) {
    char checksum[65];
    if(!safe_metadata_token(pending_kind,NOVA_PENDING_KIND_MAX) || !safe_metadata_token(update_id,NOVA_UPDATE_ID_MAX)) return 0;
    slot_metadata_checksum_v3(record_seq,active,previous,generation,confirmed,attempts_left,pending_kind,update_id,checksum);
    long fd=nova_open_write(tmp,0644); if(fd<0) return 0;
    nova_fd_putln((int)fd,"# NovaOS Stage-4D redundant boot/update journal");
    nova_fd_putln((int)fd,"format=3");
    nova_fd_puts((int)fd,"record_seq="); nova_fd_put_u64((int)fd,record_seq); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"active="); char a[2]={active,'\0'}; nova_fd_putln((int)fd,a);
    nova_fd_puts((int)fd,"previous="); char p[2]={previous,'\0'}; nova_fd_putln((int)fd,p);
    nova_fd_puts((int)fd,"generation="); nova_fd_put_u64((int)fd,generation); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"confirmed="); nova_fd_putln((int)fd,confirmed?"yes":"no");
    nova_fd_puts((int)fd,"attempts_left="); nova_fd_put_u64((int)fd,attempts_left); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"pending_kind="); nova_fd_putln((int)fd,pending_kind);
    nova_fd_puts((int)fd,"update_id="); nova_fd_putln((int)fd,update_id);
    nova_fd_puts((int)fd,"checksum="); nova_fd_putln((int)fd,checksum);
    if(nova_fsync((int)fd)<0){nova_close((int)fd);(void)nova_unlink(tmp);return 0;}
    nova_close((int)fd);
    if(nova_rename(tmp,path)<0){(void)nova_unlink(tmp);return 0;}
    nova_sync(); return 1;
}

static int commit_slot_metadata(char active, char previous, unsigned long generation, int confirmed,
                                unsigned long attempts_left, const char *pending_kind,
                                const char *update_id) {
    unsigned long next_seq=0;
    if(!checked_add_ul(g_slot_metadata_seq,1UL,&next_seq)) return 0;
    const int use_a=(next_seq&1UL)!=0;
    const char *path=use_a?NOVA_SLOT_METADATA_A:NOVA_SLOT_METADATA_B;
    const char *tmp=use_a?NOVA_SLOT_METADATA_A_TMP:NOVA_SLOT_METADATA_B_TMP;
    if(!write_slot_record(path,tmp,next_seq,active,previous,generation,confirmed,attempts_left,pending_kind,update_id)) return 0;
    g_slot_metadata_seq=next_seq; g_slot_metadata_source=use_a?'A':'B';
    return 1;
}

static int seed_dual_slot_metadata(char active, char previous, unsigned long generation, int confirmed,
                                   unsigned long attempts_left, const char *pending_kind,
                                   const char *update_id) {
    int a=write_slot_record(NOVA_SLOT_METADATA_A,NOVA_SLOT_METADATA_A_TMP,1,active,previous,generation,confirmed,attempts_left,pending_kind,update_id);
    int b=write_slot_record(NOVA_SLOT_METADATA_B,NOVA_SLOT_METADATA_B_TMP,2,active,previous,generation,confirmed,attempts_left,pending_kind,update_id);
    if(b){g_slot_metadata_seq=2;g_slot_metadata_source='B';return 1;}
    if(a){g_slot_metadata_seq=1;g_slot_metadata_source='A';g_slot_metadata_recovered=1;return 1;}
    return 0;
}

static int parse_slot_record_v3(const char *path, struct nova_slot_record *out) {
    char buffer[FILE_BUFFER], checksum[65]=""; long n=read_text_file(path,buffer,sizeof(buffer));
    if(n<0) return 0;
    nova_zero(out,sizeof(*out)); out->active='A'; out->previous='A'; out->confirmed=1;
    copy_string(out->pending_kind,sizeof(out->pending_kind),"none"); copy_string(out->update_id,sizeof(out->update_id),"none");
    int format=0; unsigned int seen=0; char *q=buffer;
    while(*q){
        char *line=q; while(*q && *q!='\n')q++; if(*q=='\n')*q++='\0'; line=trim(line);
        if(!*line || *line=='#') continue;
        char *eq=line; while(*eq && *eq!='=')eq++; if(*eq!='=')continue; *eq++='\0';
        char *key=trim(line), *value=trim(eq);
        if(nova_streq(key,"format")){int ok=0;unsigned long v=parse_unsigned(value,&ok);if(ok){format=(int)v;seen|=1U;}}
        else if(nova_streq(key,"record_seq")){int ok=0;unsigned long v=parse_unsigned(value,&ok);if(ok){out->record_seq=v;seen|=2U;}}
        else if(nova_streq(key,"active")&&(value[0]=='A'||value[0]=='B')&&value[1]=='\0'){out->active=value[0];seen|=4U;}
        else if(nova_streq(key,"previous")&&(value[0]=='A'||value[0]=='B')&&value[1]=='\0'){out->previous=value[0];seen|=8U;}
        else if(nova_streq(key,"generation")){int ok=0;unsigned long v=parse_unsigned(value,&ok);if(ok){out->generation=v;seen|=16U;}}
        else if(nova_streq(key,"confirmed")&&(nova_streq(value,"yes")||nova_streq(value,"no"))){out->confirmed=nova_streq(value,"yes");seen|=32U;}
        else if(nova_streq(key,"attempts_left")){int ok=0;unsigned long v=parse_unsigned(value,&ok);if(ok){out->attempts_left=v;seen|=64U;}}
        else if(nova_streq(key,"pending_kind")&&safe_metadata_token(value,sizeof(out->pending_kind))){copy_string(out->pending_kind,sizeof(out->pending_kind),value);seen|=128U;}
        else if(nova_streq(key,"update_id")&&safe_metadata_token(value,sizeof(out->update_id))){copy_string(out->update_id,sizeof(out->update_id),value);seen|=256U;}
        else if(nova_streq(key,"checksum")&&safe_hex(value,64)){copy_string(checksum,sizeof(checksum),value);seen|=512U;}
    }
    if(format!=3 || seen!=1023U) return 0;
    char expected[65]; slot_metadata_checksum_v3(out->record_seq,out->active,out->previous,out->generation,out->confirmed,
                                                  out->attempts_left,out->pending_kind,out->update_id,expected);
    return nova_streq(checksum,expected);
}

static int parse_legacy_slot_metadata(struct nova_slot_record *out) {
    char buffer[FILE_BUFFER], checksum[65]=""; long n=read_text_file(NOVA_SLOT_METADATA_LEGACY,buffer,sizeof(buffer));
    if(n<0) return 0;
    nova_zero(out,sizeof(*out)); out->active='A'; out->previous='A'; out->confirmed=1;
    copy_string(out->pending_kind,sizeof(out->pending_kind),"none"); copy_string(out->update_id,sizeof(out->update_id),"none");
    int format=0; char *q=buffer;
    while(*q){
        char *line=q; while(*q&&*q!='\n')q++; if(*q=='\n')*q++='\0'; line=trim(line);
        if(!*line||*line=='#')continue; char *eq=line; while(*eq&&*eq!='=')eq++; if(*eq!='=')continue; *eq++='\0';
        char *key=trim(line),*value=trim(eq);
        if(nova_streq(key,"format")){int ok=0;unsigned long v=parse_unsigned(value,&ok);if(ok)format=(int)v;}
        else if(nova_streq(key,"active")&&(value[0]=='A'||value[0]=='B')&&value[1]=='\0')out->active=value[0];
        else if(nova_streq(key,"previous")&&(value[0]=='A'||value[0]=='B')&&value[1]=='\0')out->previous=value[0];
        else if(nova_streq(key,"generation")){int ok=0;unsigned long v=parse_unsigned(value,&ok);if(ok)out->generation=v;}
        else if(nova_streq(key,"confirmed"))out->confirmed=nova_streq(value,"yes");
        else if(nova_streq(key,"attempts_left")){int ok=0;unsigned long v=parse_unsigned(value,&ok);if(ok)out->attempts_left=v;}
        else if(nova_streq(key,"pending_kind")&&safe_metadata_token(value,sizeof(out->pending_kind)))copy_string(out->pending_kind,sizeof(out->pending_kind),value);
        else if(nova_streq(key,"update_id")&&safe_metadata_token(value,sizeof(out->update_id)))copy_string(out->update_id,sizeof(out->update_id),value);
        else if(nova_streq(key,"checksum")&&safe_hex(value,64))copy_string(checksum,sizeof(checksum),value);
    }
    if(format==1){out->attempts_left=out->confirmed?0:1;copy_string(out->pending_kind,sizeof(out->pending_kind),out->confirmed?"none":"manual");copy_string(out->update_id,sizeof(out->update_id),"none");return 1;}
    if(format!=2) return 0;
    char expected[65]; slot_metadata_checksum_v2(out->active,out->previous,out->generation,out->confirmed,out->attempts_left,out->pending_kind,out->update_id,expected);
    return nova_streq(checksum,expected);
}

static void copy_slot_record(struct nova_slot_record *dst, const struct nova_slot_record *src) {
    dst->record_seq=src->record_seq; dst->active=src->active; dst->previous=src->previous;
    dst->generation=src->generation; dst->confirmed=src->confirmed; dst->attempts_left=src->attempts_left;
    copy_string(dst->pending_kind,sizeof(dst->pending_kind),src->pending_kind);
    copy_string(dst->update_id,sizeof(dst->update_id),src->update_id);
}

static void apply_slot_record(const struct nova_slot_record *r) {
    g_active_slot=r->active; g_next_slot=r->active; g_previous_slot=r->previous; g_slot_generation=r->generation;
    g_slot_confirmed=r->confirmed; g_boot_attempts_left=r->attempts_left;
    copy_string(g_pending_kind,sizeof(g_pending_kind),r->pending_kind); copy_string(g_update_id,sizeof(g_update_id),r->update_id);
}

static void load_slot_metadata(void) {
    struct nova_slot_record a,b,selected; int va=parse_slot_record_v3(NOVA_SLOT_METADATA_A,&a), vb=parse_slot_record_v3(NOVA_SLOT_METADATA_B,&b);
    int mirror_damaged=0;
    if(va && vb){
        if(a.record_seq>=b.record_seq){copy_slot_record(&selected,&a);g_slot_metadata_source='A';}
        else {copy_slot_record(&selected,&b);g_slot_metadata_source='B';}
        g_slot_metadata_seq=selected.record_seq;
    } else if(va || vb){
        if(va){copy_slot_record(&selected,&a);g_slot_metadata_source='A';}
        else {copy_slot_record(&selected,&b);g_slot_metadata_source='B';}
        g_slot_metadata_seq=selected.record_seq;
        g_slot_metadata_recovered=1; mirror_damaged=1;
        log_simple("WARN","slot","journal_mirror_recovery","one boot-state journal record was invalid; recovered from mirror");
    } else {
        g_slot_metadata_valid=0; g_slot_metadata_recovered=1;
        if(parse_legacy_slot_metadata(&selected)){
            log_simple("WARN","slot","journal_migration","legacy boot metadata migrated to redundant Stage-4C journal");
        } else {
            selected.record_seq=0; selected.active='A'; selected.previous='A'; selected.generation=0; selected.confirmed=1; selected.attempts_left=0;
            copy_string(selected.pending_kind,sizeof(selected.pending_kind),"none"); copy_string(selected.update_id,sizeof(selected.update_id),"none");
            log_simple("ERROR","slot","journal_rebuilt","no valid boot-state record remained; rebuilt safe confirmed System A state");
        }
        g_slot_metadata_seq=0; g_slot_metadata_source='-';
        if(!seed_dual_slot_metadata(selected.active,selected.previous,selected.generation,selected.confirmed,selected.attempts_left,selected.pending_kind,selected.update_id)){
            g_slot_journal_write_failed=1; g_boot_degraded=1;
            copy_string(g_evidence_error,sizeof(g_evidence_error),"slot-journal-write-failed");
            log_simple("ERROR","slot","journal_seed_failed","redundant boot-state journal could not be initialized; normal boot is blocked");
        }
    }

    apply_slot_record(&selected);
    if(mirror_damaged){
        if(!commit_slot_metadata(g_active_slot,g_previous_slot,g_slot_generation,g_slot_confirmed,g_boot_attempts_left,g_pending_kind,g_update_id))
            log_simple("WARN","slot","journal_mirror_self_heal_failed","boot-state mirror remained degraded; selected valid record remains authoritative");
    }

    if(!g_slot_confirmed && g_boot_attempts_left==0){
        char failed=g_active_slot, fallback=g_previous_slot; unsigned long next_generation=0;
        if(!checked_add_ul(g_slot_generation,1UL,&next_generation) ||
           !commit_slot_metadata(fallback,failed,next_generation,1,0,"none",g_update_id)){
            g_slot_journal_write_failed=1; g_boot_degraded=1;
            copy_string(g_evidence_error,sizeof(g_evidence_error),"slot-journal-write-failed");
            log_simple("ERROR","slot","automatic_rollback_commit_failed","pending slot exhausted boot attempts but durable rollback journal commit failed; normal boot is blocked");
        } else {
            g_active_slot=fallback; g_next_slot=fallback; g_previous_slot=failed; g_slot_generation=next_generation; g_slot_confirmed=1; g_boot_attempts_left=0;
            g_rollback_performed=1; copy_string(g_pending_kind,sizeof(g_pending_kind),"none");
            log_simple("WARN","slot","automatic_rollback","pending slot exhausted boot attempts; rolled back to previous slot");
        }
    } else if(!g_slot_confirmed && g_boot_attempts_left>0){
        unsigned long next_attempts=g_boot_attempts_left-1UL;
        if(!commit_slot_metadata(g_active_slot,g_previous_slot,g_slot_generation,0,next_attempts,g_pending_kind,g_update_id)){
            g_slot_journal_write_failed=1; g_boot_degraded=1;
            copy_string(g_evidence_error,sizeof(g_evidence_error),"slot-journal-write-failed");
            log_simple("ERROR","slot","boot_attempt_commit_failed","pending-slot attempt budget could not be durably consumed; normal boot is blocked");
        } else {
            g_boot_attempts_left=next_attempts; g_boot_attempt_consumed=1;
            log_simple("INFO","slot","boot_attempt","pending slot boot attempt consumed before trust/integrity verification");
        }
    }
}


struct nova_verity_descriptor {
    int format;
    char artifact_kind[32];
    char id[NOVA_UPDATE_ID_MAX];
    char version[NOVA_UPDATE_VERSION_MAX];
    char payload_hash[65];
    unsigned long payload_size;
    unsigned long data_block_size;
    unsigned long hash_block_size;
    unsigned long data_blocks;
    unsigned long hash_start_block;
    unsigned long rollback_index;
    int rollback_index_present;
    char hash_algorithm[16];
    char root_hash[65];
    char salt[65];
};

static int safe_hex(const char *s, nova_size_t exact_len) {
    if(!s || nova_strlen(s)!=exact_len) return 0;
    for(nova_size_t i=0;i<exact_len;i++){
        char c=s[i];
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))) return 0;
    }
    return 1;
}

static int parse_verity_descriptor_file(const char *path, struct nova_verity_descriptor *out) {
    char buffer[FILE_BUFFER]; long n=read_text_file(path,buffer,sizeof(buffer));
    if(n<0) return 0;
    nova_zero(out,sizeof(*out)); int format=0; char payload[64]="";
    char *q=buffer;
    while(*q){
        char *line=q; while(*q && *q!='\n')q++; if(*q=='\n')*q++='\0'; line=trim(line);
        if(!*line || *line=='#') continue;
        char *eq=line; while(*eq && *eq!='=')eq++; if(*eq!='=')continue; *eq++='\0';
        char *key=trim(line), *value=trim(eq);
        if(nova_streq(key,"format")){int ok=0; unsigned long v=parse_unsigned(value,&ok); if(ok)format=(int)v;}
        else if(nova_streq(key,"artifact_kind") && safe_metadata_token(value,sizeof(out->artifact_kind))) copy_string(out->artifact_kind,sizeof(out->artifact_kind),value);
        else if(nova_streq(key,"update_id") && safe_metadata_token(value,sizeof(out->id))) copy_string(out->id,sizeof(out->id),value);
        else if(nova_streq(key,"version") && safe_metadata_token(value,sizeof(out->version))) copy_string(out->version,sizeof(out->version),value);
        else if(nova_streq(key,"payload") && safe_metadata_token(value,sizeof(payload))) copy_string(payload,sizeof(payload),value);
        else if(nova_streq(key,"payload_sha256") && safe_hex(value,64)) copy_string(out->payload_hash,sizeof(out->payload_hash),value);
        else if(nova_streq(key,"payload_size")){int ok=0; unsigned long v=parse_unsigned(value,&ok); if(ok)out->payload_size=v;}
        else if(nova_streq(key,"verity_data_block_size")){int ok=0; unsigned long v=parse_unsigned(value,&ok); if(ok)out->data_block_size=v;}
        else if(nova_streq(key,"verity_hash_block_size")){int ok=0; unsigned long v=parse_unsigned(value,&ok); if(ok)out->hash_block_size=v;}
        else if(nova_streq(key,"verity_data_blocks")){int ok=0; unsigned long v=parse_unsigned(value,&ok); if(ok)out->data_blocks=v;}
        else if(nova_streq(key,"verity_hash_start_block")){int ok=0; unsigned long v=parse_unsigned(value,&ok); if(ok)out->hash_start_block=v;}
        else if(nova_streq(key,"rollback_index")){int ok=0; unsigned long v=parse_unsigned(value,&ok); if(ok){out->rollback_index=v;out->rollback_index_present=1;}}
        else if(nova_streq(key,"verity_hash_algorithm") && safe_metadata_token(value,sizeof(out->hash_algorithm))) copy_string(out->hash_algorithm,sizeof(out->hash_algorithm),value);
        else if(nova_streq(key,"verity_root_hash") && safe_hex(value,64)) copy_string(out->root_hash,sizeof(out->root_hash),value);
        else if(nova_streq(key,"verity_salt") && safe_hex(value,64)) copy_string(out->salt,sizeof(out->salt),value);
    }
    out->format=format;
    int is_system=nova_streq(out->artifact_kind,"system-slot") || nova_streq(out->artifact_kind,"system-update");
    int is_recovery=nova_streq(out->artifact_kind,"recovery-slot");
    int geometry_ok=0, payload_ok=0;
    if(is_system){
        payload_ok=nova_streq(payload,"system.slot") && out->payload_size==NOVA_SYSTEM_SLOT_BYTES;
        geometry_ok=out->data_blocks==NOVA_VERITY_DATA_BLOCKS && out->hash_start_block==NOVA_VERITY_HASH_START_BLOCK;
    } else if(is_recovery){
        payload_ok=nova_streq(payload,"recovery.slot") && out->payload_size==NOVA_RECOVERY_SLOT_BYTES;
        geometry_ok=out->data_blocks==NOVA_RECOVERY_VERITY_DATA_BLOCKS && out->hash_start_block==NOVA_RECOVERY_VERITY_HASH_START_BLOCK;
    }
    int format_ok=(format==2 || format==3 || ((format==4 || format==5) && nova_streq(out->artifact_kind,"system-update")));
    int rollback_ok=1;
    if(is_system){
        /* Stage-4C descriptors are format 3 and carry a signed monotonic rollback index.
         * Legacy format-2 System descriptors are interpreted as rollback_index=0 so a
         * previously valid Stage-4C image can never bypass a TPM counter > 0. */
        if(format>=3) rollback_ok=out->rollback_index_present;
        else { out->rollback_index=0; out->rollback_index_present=1; }
    } else if(format>=3 && !out->rollback_index_present){
        out->rollback_index=0; out->rollback_index_present=1;
    }
    return format_ok && rollback_ok && (is_system||is_recovery) && out->id[0] && out->version[0] && payload_ok &&
           safe_hex(out->payload_hash,64) && out->data_block_size==NOVA_VERITY_DATA_BLOCK_SIZE &&
           out->hash_block_size==NOVA_VERITY_HASH_BLOCK_SIZE && geometry_ok &&
           nova_streq(out->hash_algorithm,"sha256") && safe_hex(out->root_hash,64) && safe_hex(out->salt,64);
}

static int parse_recovery_descriptor_v3_strict(const char *path, struct nova_verity_descriptor *out) {
    char buffer[FILE_BUFFER]; long n=read_text_file(path,buffer,sizeof(buffer));
    if(n<0) return 0;
    nova_zero(out,sizeof(*out));
    char payload[32]="";
    unsigned long seen=0;
    char *q=buffer;
    while(*q){
        char *line=q; while(*q && *q!='\n')q++; if(*q=='\n')*q++='\0'; line=trim(line);
        if(!*line || *line=='#') continue;
        char *eq=line; while(*eq && *eq!='=')eq++;
        if(*eq!='=') return 0;
        *eq++='\0';
        char *key=trim(line), *value=trim(eq);
        unsigned long bit=0, v=0;
        if(nova_streq(key,"format")){
            bit=1UL<<0; if(!parse_unsigned_canonical(value,&v) || v!=3UL) return 0; out->format=3;
        } else if(nova_streq(key,"artifact_kind")){
            bit=1UL<<1; if(!safe_metadata_token(value,sizeof(out->artifact_kind)) || !nova_streq(value,"recovery-slot")) return 0;
            copy_string(out->artifact_kind,sizeof(out->artifact_kind),value);
        } else if(nova_streq(key,"update_id")){
            bit=1UL<<2; if(!safe_metadata_token(value,sizeof(out->id))) return 0; copy_string(out->id,sizeof(out->id),value);
        } else if(nova_streq(key,"version")){
            bit=1UL<<3; if(!safe_metadata_token(value,sizeof(out->version))) return 0; copy_string(out->version,sizeof(out->version),value);
        } else if(nova_streq(key,"rollback_index")){
            bit=1UL<<4; if(!parse_unsigned_canonical(value,&v) || v!=0UL) return 0; out->rollback_index=v; out->rollback_index_present=1;
        } else if(nova_streq(key,"payload")){
            bit=1UL<<5; if(!safe_metadata_token(value,sizeof(payload)) || !nova_streq(value,"recovery.slot")) return 0; copy_string(payload,sizeof(payload),value);
        } else if(nova_streq(key,"payload_sha256")){
            bit=1UL<<6; if(!safe_hex(value,64)) return 0; copy_string(out->payload_hash,sizeof(out->payload_hash),value);
        } else if(nova_streq(key,"payload_size")){
            bit=1UL<<7; if(!parse_unsigned_canonical(value,&v) || v!=NOVA_RECOVERY_SLOT_BYTES) return 0; out->payload_size=v;
        } else if(nova_streq(key,"verity_data_block_size")){
            bit=1UL<<8; if(!parse_unsigned_canonical(value,&v) || v!=NOVA_VERITY_DATA_BLOCK_SIZE) return 0; out->data_block_size=v;
        } else if(nova_streq(key,"verity_hash_block_size")){
            bit=1UL<<9; if(!parse_unsigned_canonical(value,&v) || v!=NOVA_VERITY_HASH_BLOCK_SIZE) return 0; out->hash_block_size=v;
        } else if(nova_streq(key,"verity_data_blocks")){
            bit=1UL<<10; if(!parse_unsigned_canonical(value,&v) || v!=NOVA_RECOVERY_VERITY_DATA_BLOCKS) return 0; out->data_blocks=v;
        } else if(nova_streq(key,"verity_hash_start_block")){
            bit=1UL<<11; if(!parse_unsigned_canonical(value,&v) || v!=NOVA_RECOVERY_VERITY_HASH_START_BLOCK) return 0; out->hash_start_block=v;
        } else if(nova_streq(key,"verity_hash_algorithm")){
            bit=1UL<<12; if(!safe_metadata_token(value,sizeof(out->hash_algorithm)) || !nova_streq(value,"sha256")) return 0;
            copy_string(out->hash_algorithm,sizeof(out->hash_algorithm),value);
        } else if(nova_streq(key,"verity_root_hash")){
            bit=1UL<<13; if(!safe_hex(value,64)) return 0; copy_string(out->root_hash,sizeof(out->root_hash),value);
        } else if(nova_streq(key,"verity_salt")){
            bit=1UL<<14; if(!safe_hex(value,64)) return 0; copy_string(out->salt,sizeof(out->salt),value);
        } else return 0;
        if(seen&bit) return 0;
        seen|=bit;
    }
    return seen==((1UL<<15)-1UL) && out->id[0] && out->version[0] && out->rollback_index_present;
}

static const char *slot_verity_manifest(char slot) {
    return slot=='B'?NOVA_SLOT_B_VERITY_MANIFEST:NOVA_SLOT_A_VERITY_MANIFEST;
}
static const char *slot_verity_signature(char slot) {
    return slot=='B'?NOVA_SLOT_B_VERITY_SIGNATURE:NOVA_SLOT_A_VERITY_SIGNATURE;
}

static int ensure_pinned_root_key(const char *cert_path) {
    if(g_trust_key>=0) return 1;
    char cert_hash[65];
    if(nova_sha256_file(cert_path,cert_hash)<0 || !nova_streq(cert_hash,NOVA_ROOT_CERT_SHA256)) return 0;
    unsigned char cert[TRUST_BUFFER]; long cert_len=read_binary_file(cert_path,cert,sizeof(cert));
    if(cert_len<=0) return 0;
    long keyring=nova_keyctl_get_keyring_id(NOVA_KEY_SPEC_PROCESS_KEYRING,1); if(keyring<0) return 0;
    long key=nova_add_key("asymmetric",NOVA_TRUST_KEY_ID,cert,(nova_size_t)cert_len,keyring); if(key<0) return 0;
    g_trust_key=key; return 1;
}


struct nova_release_policy_view {
    int format;
    unsigned long trust_epoch;
    char policy_state[24];
    char active_key_id[64];
    char active_cert_sha256[65];
    char next_key_id[64];
    char next_cert_sha256[65];
    char revoked_key_ids[256];
    char policy_sha256[65];
    long active_key;
    long next_key;
};

struct nova_update_auth_fields {
    unsigned long trust_epoch;
    char signer_key_id[64];
    char transition[24];
    char target_policy_sha256[65];
};

static int release_list_contains(const char *list, const char *id) {
    if(!list || !id || !*id || nova_streq(list,"none")) return 0;
    const char *p=list;
    while(*p){
        const char *start=p; while(*p && *p!=',')p++;
        nova_size_t n=(nova_size_t)(p-start), m=nova_strlen(id);
        if(n==m){nova_size_t i=0;while(i<n && start[i]==id[i])i++;if(i==n)return 1;}
        if(*p==',')p++;
    }
    return 0;
}

static int release_revocation_list_valid(const char *list) {
    if(!list || !*list) return 0;
    if(nova_streq(list,"none")) return 1;
    const char *p=list;
    while(*p){
        const char *start=p; while(*p && *p!=',')p++;
        nova_size_t n=(nova_size_t)(p-start);
        if(n==0 || n>=64) return 0;
        char token[64]; for(nova_size_t i=0;i<n;i++)token[i]=start[i]; token[n]='\0';
        if(!safe_metadata_token(token,sizeof(token)) || nova_streq(token,"none")) return 0;
        const char *q=list;
        while(q<start){
            const char *qs=q; while(*q && *q!=',')q++;
            nova_size_t qn=(nova_size_t)(q-qs);
            if(qn==n){nova_size_t i=0;while(i<n && qs[i]==start[i])i++;if(i==n)return 0;}
            if(*q==',')q++;
        }
        if(*p==',')p++;
    }
    return 1;
}

static int release_revocation_superset(const char *candidate, const char *required) {
    if(!required || !*required || nova_streq(required,"none")) return 1;
    if(!candidate || !*candidate || nova_streq(candidate,"none")) return 0;
    const char *p=required;
    while(*p){
        const char *start=p; while(*p && *p!=',')p++;
        nova_size_t n=(nova_size_t)(p-start);
        if(n==0 || n>=64) return 0;
        char id[64]; for(nova_size_t i=0;i<n;i++)id[i]=start[i]; id[n]='\0';
        if(!release_list_contains(candidate,id)) return 0;
        if(*p==',')p++;
    }
    return 1;
}

static int release_epoch_bounds(unsigned long *committed_epoch, unsigned long *pending_epoch) {
    unsigned long committed=0,pending=0;
    if(g_tpm_state!=TPM_STATE_READY) return 0;
    if(!checked_add_ul(g_tpm_trust_epoch_counter,1UL,&committed) || !checked_add_ul(committed,1UL,&pending)) return 0;
    if(committed_epoch) *committed_epoch=committed;
    if(pending_epoch) *pending_epoch=pending;
    return 1;
}

static int release_policy_epoch_allowed(unsigned long epoch) {
    unsigned long committed=0,pending=0;
    if(epoch==0 || !release_epoch_bounds(&committed,&pending)) return 0;
    return epoch==committed || epoch==pending;
}

static const char *release_epoch_state(unsigned long epoch) {
    unsigned long committed=0,pending=0;
    if(epoch==0 || !release_epoch_bounds(&committed,&pending)) return "UNAVAILABLE";
    if(epoch==committed) return "COMMITTED";
    if(epoch==pending) return "PENDING_TPM_COMMIT";
    if(epoch<committed) return "STALE";
    return "FUTURE";
}

static int parse_release_policy_file(const char *path, struct nova_release_policy_view *out) {
    char buffer[FILE_BUFFER], active_ref[64]="", next_ref[64]="", binding[128]="", policy_id[64]="", sig_alg[40]="";
    long n=read_text_file(path,buffer,sizeof(buffer)); if(n<0) return 0;
    nova_zero(out,sizeof(*out)); out->active_key=-1; out->next_key=-1;
    copy_string(out->active_key_id,sizeof(out->active_key_id),"none");
    copy_string(out->next_key_id,sizeof(out->next_key_id),"none");
    copy_string(out->revoked_key_ids,sizeof(out->revoked_key_ids),"none");
    copy_string(out->policy_state,sizeof(out->policy_state),"unavailable");
    unsigned long seen=0; int trust_epoch_canonical=0; char *q=buffer;
    while(*q){
        char *line=q; while(*q&&*q!='\n')q++; if(*q=='\n')*q++='\0'; line=trim(line);
        if(!*line||*line=='#')continue;
        char *eq=line; while(*eq&&*eq!='=')eq++; if(*eq!='=')return 0; *eq++='\0';
        char *key=trim(line),*value=trim(eq); unsigned long bit=0;
        if(nova_streq(key,"format")){bit=1UL;if(seen&bit)return 0;int ok=0;unsigned long v=parse_unsigned(value,&ok);if(!ok)return 0;out->format=(int)v;}
        else if(nova_streq(key,"trust_epoch")){bit=2UL;if(seen&bit)return 0;int ok=0;unsigned long v=parse_unsigned(value,&ok),canonical=0;if(!ok||v==0)return 0;trust_epoch_canonical=parse_unsigned_canonical(value,&canonical)&&canonical==v;out->trust_epoch=v;}
        else if(nova_streq(key,"active_key_id")){bit=4UL;if(seen&bit)return 0;if(!safe_metadata_token(value,sizeof(out->active_key_id)))return 0;copy_string(out->active_key_id,sizeof(out->active_key_id),value);}
        else if(nova_streq(key,"active_cert")){bit=8UL;if(seen&bit)return 0;if(!safe_metadata_token(value,sizeof(active_ref))&&!nova_streq(value,"/System/Trust/release-active.der"))return 0;copy_string(active_ref,sizeof(active_ref),value);}
        else if(nova_streq(key,"active_cert_sha256")){bit=16UL;if(seen&bit)return 0;if(!safe_hex(value,64))return 0;copy_string(out->active_cert_sha256,sizeof(out->active_cert_sha256),value);}
        else if(nova_streq(key,"next_key_id")){bit=32UL;if(seen&bit)return 0;if(!safe_metadata_token(value,sizeof(out->next_key_id)))return 0;copy_string(out->next_key_id,sizeof(out->next_key_id),value);}
        else if(nova_streq(key,"next_cert")){bit=64UL;if(seen&bit)return 0;if(!nova_streq(value,"none")&&!safe_metadata_token(value,sizeof(next_ref))&&!nova_streq(value,"/System/Trust/release-next.der"))return 0;copy_string(next_ref,sizeof(next_ref),value);}
        else if(nova_streq(key,"next_cert_sha256")){bit=128UL;if(seen&bit)return 0;if(!nova_streq(value,"none")&&!safe_hex(value,64))return 0;copy_string(out->next_cert_sha256,sizeof(out->next_cert_sha256),value);}
        else if(nova_streq(key,"revoked_key_ids")){bit=256UL;if(seen&bit)return 0;if(nova_strlen(value)>=sizeof(out->revoked_key_ids)||!release_revocation_list_valid(value))return 0;copy_string(out->revoked_key_ids,sizeof(out->revoked_key_ids),value);}
        else if(nova_streq(key,"policy_binding")){bit=512UL;if(seen&bit)return 0;if(nova_strlen(value)>=sizeof(binding))return 0;copy_string(binding,sizeof(binding),value);}
        else if(nova_streq(key,"policy_id")){bit=1024UL;if(seen&bit)return 0;if(!safe_metadata_token(value,sizeof(policy_id)))return 0;copy_string(policy_id,sizeof(policy_id),value);}
        else if(nova_streq(key,"policy_state")){bit=2048UL;if(seen&bit)return 0;if(!safe_metadata_token(value,sizeof(out->policy_state)))return 0;copy_string(out->policy_state,sizeof(out->policy_state),value);}
        else if(nova_streq(key,"signature_algorithm")){bit=4096UL;if(seen&bit)return 0;if(!safe_metadata_token(value,sizeof(sig_alg)))return 0;copy_string(sig_alg,sizeof(sig_alg),value);}
        else return 0;
        seen|=bit;
    }
    if(out->format==2){
        if((seen&1023UL)!=1023UL || (seen&~1023UL)!=0) return 0;
        if(!nova_streq(binding,"root-detached-signature+root-signed-system-manifest+dm-verity+TPM-trust-epoch") &&
           !nova_streq(binding,"root-signed-system-manifest+dm-verity+TPM-antirollback")) return 0;
        copy_string(out->policy_state,sizeof(out->policy_state),nova_streq(out->next_key_id,"none")?"stable":"rotation-prepared");
    } else if(out->format==3){
        if(seen!=8191UL || !trust_epoch_canonical) return 0;
        if(!nova_streq(policy_id,"novaos-release-policy") || !nova_streq(sig_alg,"rsa-pkcs1-sha256")) return 0;
        if(!nova_streq(binding,"root-detached-signature+root-signed-system-manifest+dm-verity+TPM-trust-epoch")) return 0;
        if(!nova_streq(active_ref,"release-active.der")) return 0;
        if(!nova_streq(out->policy_state,nova_streq(out->next_key_id,"none")?"stable":"rotation-prepared")) return 0;
    } else return 0;
    if(release_list_contains(out->revoked_key_ids,out->active_key_id)) return 0;
    if(!nova_streq(out->next_key_id,"none")){
        if(nova_streq(out->next_key_id,out->active_key_id) || release_list_contains(out->revoked_key_ids,out->next_key_id)) return 0;
        if(out->format==3 && !nova_streq(next_ref,"release-next.der")) return 0;
        if(!safe_hex(out->next_cert_sha256,64)) return 0;
    } else {
        if(!nova_streq(next_ref,"none") || !nova_streq(out->next_cert_sha256,"none")) return 0;
    }
    return 1;
}

static long load_or_reuse_release_key(const char *key_id, const char *cert_path, const char *expected_sha256) {
    char actual[65];
    if(!key_id || !expected_sha256 || nova_sha256_file(cert_path,actual)<0 || !nova_streq(actual,expected_sha256)) return -1;

    /* add_key(2) may update or displace an existing key with the same
     * type+description in the destination keyring.  Re-loading a target
     * release policy during `update verify` must therefore not invalidate the
     * g_release_key/g_release_next_key serials that authorize a later
     * `update apply` in the same boot session.  Reuse is safe only after the
     * candidate certificate file hash has matched the already-authenticated
     * policy digest and key ID. */
    if(g_release_key>=0 && nova_streq(key_id,g_release_active_key_id) &&
       nova_streq(expected_sha256,g_release_active_cert_sha256)) return g_release_key;
    if(g_release_next_key>=0 && nova_streq(key_id,g_release_next_key_id) &&
       nova_streq(expected_sha256,g_release_next_cert_sha256)) return g_release_next_key;

    unsigned char cert[TRUST_BUFFER];
    long cert_len=read_binary_file(cert_path,cert,sizeof(cert)); if(cert_len<=0) return -1;
    long keyring=nova_keyctl_get_keyring_id(NOVA_KEY_SPEC_PROCESS_KEYRING,1); if(keyring<0) return -1;
    return nova_add_key("asymmetric",key_id,cert,(nova_size_t)cert_len,keyring);
}

static int verify_release_policy_bundle(const char *policy_path, const char *signature_path,
                                        const char *active_cert_path, const char *next_cert_path,
                                        int enforce_tpm_epoch, struct nova_release_policy_view *out) {
    if(!out || !ensure_pinned_root_key(PREBOOT_ROOT_CERT_PATH)) return 0;
    if(!verify_signature_with_loaded_root(policy_path,signature_path)) return 0;
    if(!parse_release_policy_file(policy_path,out)) return 0;
    if(enforce_tpm_epoch && !release_policy_epoch_allowed(out->trust_epoch)) return 0;
    if(nova_sha256_file(policy_path,out->policy_sha256)<0) return 0;
    out->active_key=load_or_reuse_release_key(out->active_key_id,active_cert_path,out->active_cert_sha256);
    if(out->active_key<0) return 0;
    if(!nova_streq(out->next_key_id,"none")){
        out->next_key=load_or_reuse_release_key(out->next_key_id,next_cert_path,out->next_cert_sha256);
        if(out->next_key<0) return 0;
    }
    return 1;
}

static void apply_release_policy_view(const struct nova_release_policy_view *p) {
    g_release_policy_format=p->format; g_release_trust_epoch=p->trust_epoch;
    copy_string(g_release_policy_state,sizeof(g_release_policy_state),p->policy_state);
    copy_string(g_release_policy_sha256,sizeof(g_release_policy_sha256),p->policy_sha256);
    copy_string(g_release_active_key_id,sizeof(g_release_active_key_id),p->active_key_id);
    copy_string(g_release_active_cert_sha256,sizeof(g_release_active_cert_sha256),p->active_cert_sha256);
    copy_string(g_release_next_key_id,sizeof(g_release_next_key_id),p->next_key_id);
    copy_string(g_release_next_cert_sha256,sizeof(g_release_next_cert_sha256),p->next_cert_sha256);
    copy_string(g_release_revoked_key_ids,sizeof(g_release_revoked_key_ids),p->revoked_key_ids);
    g_release_key=p->active_key; g_release_next_key=p->next_key;
}

static const char *slot_release_policy(char slot){return slot=='B'?NOVA_SLOT_B_RELEASE_POLICY:NOVA_SLOT_A_RELEASE_POLICY;}
static const char *slot_release_policy_signature(char slot){return slot=='B'?NOVA_SLOT_B_RELEASE_POLICY_SIGNATURE:NOVA_SLOT_A_RELEASE_POLICY_SIGNATURE;}
static const char *slot_release_cert(char slot){return slot=='B'?NOVA_SLOT_B_RELEASE_CERT:NOVA_SLOT_A_RELEASE_CERT;}
static const char *slot_release_next_cert(char slot){return slot=='B'?NOVA_SLOT_B_RELEASE_NEXT_CERT:NOVA_SLOT_A_RELEASE_NEXT_CERT;}

static int parse_update_manifest_v5(const char *path, struct nova_verity_descriptor *verity,
                                    struct nova_update_auth_fields *auth) {
    char buffer[FILE_BUFFER]; long n=read_text_file(path,buffer,sizeof(buffer)); if(n<0)return 0;
    nova_zero(verity,sizeof(*verity)); nova_zero(auth,sizeof(*auth));
    unsigned long seen=0; char *q=buffer;
    while(*q){
        char *line=q; while(*q&&*q!='\n')q++; if(*q=='\n')*q++='\0'; line=trim(line);
        if(!*line||*line=='#')continue;
        char *eq=line; while(*eq&&*eq!='=')eq++; if(*eq!='=')return 0; *eq++='\0';
        char *key=trim(line),*value=trim(eq); unsigned long bit=0, v=0;
        if(nova_streq(key,"format")){bit=1UL<<0;if(!parse_unsigned_canonical(value,&v)||v!=5)return 0;verity->format=5;}
        else if(nova_streq(key,"artifact_kind")){bit=1UL<<1;if(!safe_metadata_token(value,sizeof(verity->artifact_kind))||!nova_streq(value,"system-update"))return 0;copy_string(verity->artifact_kind,sizeof(verity->artifact_kind),value);}
        else if(nova_streq(key,"update_id")){bit=1UL<<2;if(!safe_metadata_token(value,sizeof(verity->id)))return 0;copy_string(verity->id,sizeof(verity->id),value);}
        else if(nova_streq(key,"version")){bit=1UL<<3;if(!safe_metadata_token(value,sizeof(verity->version)))return 0;copy_string(verity->version,sizeof(verity->version),value);}
        else if(nova_streq(key,"rollback_index")){bit=1UL<<4;if(!parse_unsigned_canonical(value,&v))return 0;verity->rollback_index=v;verity->rollback_index_present=1;}
        else if(nova_streq(key,"trust_epoch")){bit=1UL<<5;if(!parse_unsigned_canonical(value,&v)||v==0)return 0;auth->trust_epoch=v;}
        else if(nova_streq(key,"signer_key_id")){bit=1UL<<6;if(!safe_metadata_token(value,sizeof(auth->signer_key_id)))return 0;copy_string(auth->signer_key_id,sizeof(auth->signer_key_id),value);}
        else if(nova_streq(key,"transition")){bit=1UL<<7;if(!(nova_streq(value,"none")||nova_streq(value,"promote-next")))return 0;copy_string(auth->transition,sizeof(auth->transition),value);}
        else if(nova_streq(key,"target_release_policy_sha256")){bit=1UL<<8;if(!safe_hex(value,64))return 0;copy_string(auth->target_policy_sha256,sizeof(auth->target_policy_sha256),value);}
        else if(nova_streq(key,"payload")){bit=1UL<<9;if(!safe_metadata_token(value,64U)||!nova_streq(value,"system.slot"))return 0;}
        else if(nova_streq(key,"payload_sha256")){bit=1UL<<10;if(!safe_hex(value,64))return 0;copy_string(verity->payload_hash,sizeof(verity->payload_hash),value);}
        else if(nova_streq(key,"payload_size")){bit=1UL<<11;if(!parse_unsigned_canonical(value,&v)||v!=NOVA_SYSTEM_SLOT_BYTES)return 0;verity->payload_size=v;}
        else if(nova_streq(key,"verity_data_block_size")){bit=1UL<<12;if(!parse_unsigned_canonical(value,&v)||v!=NOVA_VERITY_DATA_BLOCK_SIZE)return 0;verity->data_block_size=v;}
        else if(nova_streq(key,"verity_hash_block_size")){bit=1UL<<13;if(!parse_unsigned_canonical(value,&v)||v!=NOVA_VERITY_HASH_BLOCK_SIZE)return 0;verity->hash_block_size=v;}
        else if(nova_streq(key,"verity_data_blocks")){bit=1UL<<14;if(!parse_unsigned_canonical(value,&v)||v!=NOVA_VERITY_DATA_BLOCKS)return 0;verity->data_blocks=v;}
        else if(nova_streq(key,"verity_hash_start_block")){bit=1UL<<15;if(!parse_unsigned_canonical(value,&v)||v!=NOVA_VERITY_HASH_START_BLOCK)return 0;verity->hash_start_block=v;}
        else if(nova_streq(key,"verity_hash_algorithm")){bit=1UL<<16;if(!safe_metadata_token(value,sizeof(verity->hash_algorithm))||!nova_streq(value,"sha256"))return 0;copy_string(verity->hash_algorithm,sizeof(verity->hash_algorithm),value);}
        else if(nova_streq(key,"verity_root_hash")){bit=1UL<<17;if(!safe_hex(value,64))return 0;copy_string(verity->root_hash,sizeof(verity->root_hash),value);}
        else if(nova_streq(key,"verity_salt")){bit=1UL<<18;if(!safe_hex(value,64))return 0;copy_string(verity->salt,sizeof(verity->salt),value);}
        else return 0;
        if(seen&bit)return 0; seen|=bit;
    }
    return seen==((1UL<<19)-1UL) && verity->id[0] && verity->version[0] && verity->rollback_index_present;
}

static int verify_delegated_slot_descriptor(char slot, const char *manifest, const char *signature,
                                            const struct nova_verity_descriptor *desc, int set_active_state) {
    if(desc->format!=5 || !nova_streq(desc->artifact_kind,"system-update")) return 0;
    struct nova_release_policy_view policy; struct nova_update_auth_fields auth; struct nova_verity_descriptor strict_verity;
    if(!verify_release_policy_bundle(slot_release_policy(slot),slot_release_policy_signature(slot),
                                     slot_release_cert(slot),slot_release_next_cert(slot),1,&policy)) return 0;
    if(!parse_update_manifest_v5(manifest,&strict_verity,&auth)) return 0;
    if(strict_verity.rollback_index!=desc->rollback_index || !nova_streq(strict_verity.id,desc->id) ||
       !nova_streq(strict_verity.payload_hash,desc->payload_hash) || !nova_streq(strict_verity.root_hash,desc->root_hash)) return 0;
    if(auth.trust_epoch!=policy.trust_epoch || !nova_streq(auth.signer_key_id,policy.active_key_id) ||
       !nova_streq(auth.target_policy_sha256,policy.policy_sha256) || release_list_contains(policy.revoked_key_ids,auth.signer_key_id)) return 0;
    if(!verify_signature_with_key(policy.active_key,manifest,signature)) return 0;
    if(set_active_state){
        g_preboot_release_policy_verified=1;
        copy_string(g_preboot_release_policy_sha256,sizeof(g_preboot_release_policy_sha256),policy.policy_sha256);
        g_active_slot_descriptor_format=desc->format;
        copy_string(g_active_slot_descriptor_kind,sizeof(g_active_slot_descriptor_kind),desc->artifact_kind);
    }
    return 1;
}

static int verify_slot_verity_descriptor_ex(char slot, struct nova_verity_descriptor *out, int set_active_state) {
    const char *manifest=slot_verity_manifest(slot), *signature=slot_verity_signature(slot);
    if(!ensure_pinned_root_key(PREBOOT_ROOT_CERT_PATH)) return 0;
    if(!parse_verity_descriptor_file(manifest,out)) return 0;
    int signature_ok=0;
    if(out->format==5 && nova_streq(out->artifact_kind,"system-update"))
        signature_ok=verify_delegated_slot_descriptor(slot,manifest,signature,out,set_active_state);
    else
        signature_ok=verify_signature_with_loaded_root(manifest,signature);
    if(!signature_ok) return 0;
    if(set_active_state){
        g_verity_descriptor_verified=1;
        g_active_slot_descriptor_format=out->format;
        copy_string(g_active_slot_descriptor_kind,sizeof(g_active_slot_descriptor_kind),out->artifact_kind);
        copy_string(g_verity_root_hash,sizeof(g_verity_root_hash),out->root_hash);
        copy_string(g_verity_salt,sizeof(g_verity_salt),out->salt);
    }
    return 1;
}
static int verify_slot_verity_descriptor(char slot, struct nova_verity_descriptor *out) {
    return verify_slot_verity_descriptor_ex(slot,out,1);
}

static int verify_recovery_verity_descriptor(struct nova_verity_descriptor *out) {
    if(!ensure_pinned_root_key(PREBOOT_ROOT_CERT_PATH)) return 0;
    const char *manifest=NOVA_RECOVERY_VERITY_MANIFEST_EMBEDDED;
    const char *signature=NOVA_RECOVERY_VERITY_SIGNATURE_EMBEDDED;
    long embedded=nova_open_ro(manifest);
    if(embedded>=0){
        nova_close((int)embedded);
        copy_string(g_recovery_descriptor_source,sizeof(g_recovery_descriptor_source),"firmware-initramfs");
        /* If the firmware-bound descriptor exists but is malformed/tampered,
         * never downgrade to a mutable Data copy. */
    } else {
        manifest=NOVA_RECOVERY_VERITY_MANIFEST;
        signature=NOVA_RECOVERY_VERITY_SIGNATURE;
        copy_string(g_recovery_descriptor_source,sizeof(g_recovery_descriptor_source),"data-legacy");
    }
    if(!verify_signature_with_loaded_root(manifest,signature)) return 0;
    if(nova_streq(g_recovery_descriptor_source,"firmware-initramfs")){
        if(!parse_recovery_descriptor_v3_strict(manifest,out)) return 0;
    } else {
        /* Stage-4C compatibility: only the legacy Data fallback keeps the
         * older permissive format-2/3 descriptor parser.  The Stage-4D
         * firmware-bound descriptor is always strict and non-ambiguous. */
        if(!parse_verity_descriptor_file(manifest,out)) return 0;
        if(!nova_streq(out->artifact_kind,"recovery-slot")) return 0;
    }
    g_recovery_verity_descriptor_verified=1;
    copy_string(g_recovery_verity_root_hash,sizeof(g_recovery_verity_root_hash),out->root_hash);
    copy_string(g_recovery_verity_salt,sizeof(g_recovery_verity_salt),out->salt);
    return 1;
}

static int schedule_antirollback_fallback(char failed_slot, const char *reason) {
    char fallback=(failed_slot=='A')?'B':'A';
    struct nova_verity_descriptor candidate;
    if(!verify_slot_verity_descriptor_ex(fallback,&candidate,0)){
        log_simple("ERROR","rollback","fallback_descriptor_invalid","alternate slot could not be authenticated after anti-rollback rejection");
        return 0;
    }
    unsigned long fallback_max=0;
    if(g_tpm_state!=TPM_STATE_READY || candidate.rollback_index<g_tpm_rollback_counter ||
       !checked_add_ul(g_tpm_rollback_counter,1UL,&fallback_max) || candidate.rollback_index>fallback_max){
        log_simple("ERROR","rollback","fallback_index_invalid","alternate slot does not satisfy TPM anti-rollback policy or rollback counter space is exhausted");
        return 0;
    }
    unsigned long next_generation=0;
    if(!checked_add_ul(g_slot_generation,1UL,&next_generation) ||
       !commit_slot_metadata(fallback,failed_slot,next_generation,1,0,"none",candidate.id)) return 0;
    g_next_slot=fallback; g_previous_slot=failed_slot; g_slot_generation=next_generation;
    g_slot_confirmed=1; g_boot_attempts_left=0; g_rollback_performed=1; g_verity_rollback_scheduled=1;
    copy_string(g_pending_kind,sizeof(g_pending_kind),"none"); copy_string(g_update_id,sizeof(g_update_id),candidate.id);
    log_simple("ERROR","rollback","antirollback_fallback_scheduled",reason);
    return 1;
}

static int antirollback_accept_active_descriptor(const struct nova_verity_descriptor *desc) {
    if(g_tpm_state!=TPM_STATE_READY){
        g_antirollback_rejected=1;
        log_simple("ERROR","rollback","tpm_unavailable","active System cannot be accepted without TPM anti-rollback state");
        return 0;
    }
    g_active_rollback_index=desc->rollback_index;
    if(desc->rollback_index<g_tpm_rollback_counter){
        g_antirollback_rejected=1;
        (void)schedule_antirollback_fallback(g_active_slot,"active System rollback index is below TPM monotonic counter");
        log_simple("ERROR","rollback","stale_system_rejected","signed but stale System image rejected by TPM monotonic counter");
        return 0;
    }
    unsigned long pending_rollback=0;
    if(!checked_add_ul(g_tpm_rollback_counter,1UL,&pending_rollback) || desc->rollback_index>pending_rollback){
        g_antirollback_rejected=1;
        log_simple("ERROR","rollback","future_index_rejected","System rollback index skipped more than one TPM counter generation or counter space is exhausted");
        return 0;
    }
    return 1;
}

static nova_size_t append_text(char *dst, nova_size_t cap, nova_size_t at, const char *src) {
    while(src && *src && at+1<cap) dst[at++]=*src++;
    if(at<cap) dst[at]='\0';
    return at;
}
static nova_size_t append_u64_text(char *dst, nova_size_t cap, nova_size_t at, unsigned long value) {
    char buf[32]; nova_size_t n=0;
    if(value==0) buf[n++]='0';
    else { while(value && n<sizeof(buf)){buf[n++]=(char)('0'+value%10);value/=10;} }
    while(n && at+1<cap) dst[at++]=buf[--n];
    if(at<cap) dst[at]='\0';
    return at;
}

static void dm_init_ioctl(struct nova_dm_ioctl *io, unsigned int data_size, const char *name) {
    nova_zero(io,data_size);
    io->version[0]=4; io->version[1]=48; io->version[2]=0;
    io->data_size=data_size; io->data_start=(unsigned int)sizeof(struct nova_dm_ioctl);
    if(name) copy_string(io->name,sizeof(io->name),name);
}

static int setup_verity_mapping_named(const char *raw_device, const struct nova_verity_descriptor *desc,
                                      const char *mapper_name, const char *node_path) {
    ensure_dir("/dev/mapper");
    long ctl=nova_open_rw(NOVA_DM_CONTROL);
    if(ctl<0){ (void)nova_mknod(NOVA_DM_CONTROL,NOVA_S_IFCHR|0600,10,236); ctl=nova_open_rw(NOVA_DM_CONTROL); }
    if(ctl<0) return 0;

    struct nova_dm_ioctl create; dm_init_ioctl(&create,sizeof(create),mapper_name); create.flags=NOVA_DM_READONLY_FLAG;
    if(nova_ioctl((int)ctl,NOVA_DM_DEV_CREATE,&create)<0){nova_close((int)ctl);return 0;}
    unsigned long devenc=(unsigned long)create.dev;
    unsigned long dm_major=(devenc & 0xfff00UL)>>8;
    unsigned long dm_minor=(devenc & 0xffUL)|((devenc>>12)&0xfff00UL);

    unsigned char tablebuf[4096]; nova_zero(tablebuf,sizeof(tablebuf));
    struct nova_dm_ioctl *io=(struct nova_dm_ioctl*)tablebuf;
    dm_init_ioctl(io,sizeof(tablebuf),mapper_name); io->target_count=1; io->flags=NOVA_DM_READONLY_FLAG;
    struct nova_dm_target_spec *spec=(struct nova_dm_target_spec*)(tablebuf+sizeof(struct nova_dm_ioctl));
    spec->sector_start=0;
    spec->length=(unsigned long long)desc->data_blocks*(unsigned long long)(desc->data_block_size/512UL);
    copy_string(spec->target_type,sizeof(spec->target_type),"verity");
    char *params=(char*)(spec+1); nova_size_t cap=sizeof(tablebuf)-(nova_size_t)(params-(char*)tablebuf), at=0;
    at=append_text(params,cap,at,"1 "); at=append_text(params,cap,at,raw_device); at=append_text(params,cap,at," ");
    at=append_text(params,cap,at,raw_device); at=append_text(params,cap,at," ");
    at=append_u64_text(params,cap,at,desc->data_block_size); at=append_text(params,cap,at," ");
    at=append_u64_text(params,cap,at,desc->hash_block_size); at=append_text(params,cap,at," ");
    at=append_u64_text(params,cap,at,desc->data_blocks); at=append_text(params,cap,at," ");
    at=append_u64_text(params,cap,at,desc->hash_start_block); at=append_text(params,cap,at," ");
    at=append_text(params,cap,at,desc->hash_algorithm); at=append_text(params,cap,at," ");
    at=append_text(params,cap,at,desc->root_hash); at=append_text(params,cap,at," ");
    at=append_text(params,cap,at,desc->salt);
    if(at+1>=cap){nova_close((int)ctl);return 0;}
    unsigned int used=(unsigned int)(sizeof(struct nova_dm_target_spec)+at+1);
    spec->next=(used+7U)&~7U;
    io->data_size=(unsigned int)sizeof(struct nova_dm_ioctl)+spec->next;
    if(nova_ioctl((int)ctl,NOVA_DM_TABLE_LOAD,io)<0){nova_close((int)ctl);return 0;}

    struct nova_dm_ioctl resume; dm_init_ioctl(&resume,sizeof(resume),mapper_name);
    if(nova_ioctl((int)ctl,NOVA_DM_DEV_SUSPEND,&resume)<0){nova_close((int)ctl);return 0;}
    nova_close((int)ctl);
    (void)nova_unlink(node_path);
    if(nova_mknod(node_path,NOVA_S_IFBLK|0600,dm_major,dm_minor)<0){
        long probe=nova_open_ro(node_path); if(probe<0) return 0; nova_close((int)probe);
    }
    return 1;
}

static int setup_system_verity_mapping(const char *raw_device, const struct nova_verity_descriptor *desc) {
    if(!setup_verity_mapping_named(raw_device,desc,"nova-system",NOVA_DM_NODE)) return 0;
    g_verity_mapping_active=1; g_verity=VERITY_ENFORCED; return 1;
}

static int setup_recovery_verity_mapping(const struct nova_verity_descriptor *desc) {
    if(!setup_verity_mapping_named(NOVA_RECOVERY_DEV,desc,"nova-recovery",NOVA_RECOVERY_DM_NODE)) return 0;
    g_recovery_verity_mapping_active=1; g_recovery_verity=VERITY_ENFORCED; return 1;
}


static void schedule_verity_rollback(const char *reason) {
    if(g_slot_confirmed || g_previous_slot==g_active_slot) return;
    char failed=g_active_slot, fallback=g_previous_slot;
    unsigned long next_generation=0;
    if(checked_add_ul(g_slot_generation,1UL,&next_generation) &&
       commit_slot_metadata(fallback,failed,next_generation,1,0,"none",g_update_id)){
        g_next_slot=fallback; g_slot_generation=next_generation; g_verity_rollback_scheduled=1; g_rollback_performed=1;
        log_simple("ERROR","verity","rollback_scheduled",reason);
    }
}

static void mount_stage4_storage(void) {
    g_boot_stage="STORAGE_MOUNT";
    if(nova_getpid()!=1) return;

    int data_persistent=0, system_ok=0;
    if(!nova_discover_storage_devices()){
        g_boot_degraded=1;
        log_simple("ERROR","storage","partition_discovery_failed",
                   "no unique complete NovaOS GPT partition set matched PARTUUID/PARTNAME; normal boot remains blocked");
    } else {
        nova_puts("[novad][INFO][storage] validated block disk: ");
        nova_putln(NOVA_DATA_DEV);
    }
    if(nova_mount(NOVA_DATA_DEV,"/Data","ext4",NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC,(void*)0)<0){
        (void)nova_mount("tmpfs","/Data","tmpfs",0,"mode=0755");
        g_data_writable=probe_writable("/Data/.nova-rw-probe");
        g_storage_persistent=0; g_verity=VERITY_UNAVAILABLE; g_boot_degraded=1;
        log_simple("ERROR","storage","data_unavailable","persistent Data could not be mounted; System slot metadata is unavailable but firmware-bound Recovery verification will still be attempted");
    } else {
        data_persistent=1; g_storage_persistent=1;
        ensure_dir("/Data/Nova"); ensure_dir("/Data/Users"); ensure_dir("/Data/Nova/Slots");
        ensure_dir("/Data/Nova/Slots/A"); ensure_dir("/Data/Nova/Slots/B"); ensure_dir(NOVA_BOOT_STATE_DIR); ensure_dir("/Data/Nova/Recovery");
        g_data_writable=probe_writable("/Data/.nova-rw-probe");
    }

    /* TPM availability is firmware/initramfs state, not Data state.  Keeping
     * this initialization independent lets diagnostic Recovery report the
     * monotonic state even when persistent disk metadata is unavailable. */
    initialize_tpm_policy();

    if(data_persistent){
        load_slot_metadata();
        g_system_device=(g_active_slot=='B')?NOVA_SYSTEM_B_DEV:NOVA_SYSTEM_A_DEV;
        struct nova_verity_descriptor vdesc;
        int descriptor_ok=verify_slot_verity_descriptor(g_active_slot,&vdesc);
        if(descriptor_ok && !antirollback_accept_active_descriptor(&vdesc)){
            descriptor_ok=0; g_verity_descriptor_verified=0;
        }
        if(!descriptor_ok){
            g_verity=VERITY_FAILED; g_boot_degraded=1;
            schedule_verity_rollback("active slot verity descriptor was not authenticated; rollback scheduled");
            log_simple("ERROR","verity","descriptor_rejected","active System slot has no valid signed verity descriptor");
        }
        int map_ok=descriptor_ok && setup_system_verity_mapping(g_system_device,&vdesc);
        if(descriptor_ok && !map_ok){
            g_verity=VERITY_FAILED; g_boot_degraded=1;
            schedule_verity_rollback("dm-verity mapping creation failed for pending slot; rollback scheduled");
            log_simple("ERROR","verity","mapping_failed","dm-verity mapping could not be activated");
        }
        system_ok=map_ok && nova_mount(NOVA_DM_NODE,"/System","ext4",NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV,(void*)0)>=0;
        if(map_ok && !system_ok){
            g_verity=VERITY_FAILED; g_boot_degraded=1;
            schedule_verity_rollback("dm-verity rejected System blocks while mounting pending slot; rollback scheduled");
            log_simple("ERROR","verity","mount_rejected","verified System mapping could not be mounted; corruption or filesystem damage detected");
        }
    } else {
        g_system_device="unavailable-without-data";
        g_verity=VERITY_UNAVAILABLE;
        log_simple("WARN","verity","system_metadata_unavailable","System slot descriptor/journal transport is unavailable because persistent Data is not mounted; normal boot remains blocked");
    }

    /* Recovery descriptor is firmware-bound in Stage-4D.  The signed Data copy
     * remains only as a backward-compatible fallback when the embedded file is
     * absent; an invalid embedded descriptor never downgrades to Data. */
    struct nova_verity_descriptor rdesc;
    int recovery_descriptor_ok=verify_recovery_verity_descriptor(&rdesc);
    if(!recovery_descriptor_ok){
        g_recovery_verity=VERITY_FAILED; g_boot_degraded=1;
        log_simple("ERROR","recovery","descriptor_rejected","Recovery has no valid root-signed dm-verity descriptor");
    }
    int recovery_map_ok=recovery_descriptor_ok && setup_recovery_verity_mapping(&rdesc);
    if(recovery_descriptor_ok && !recovery_map_ok){
        g_recovery_verity=VERITY_FAILED; g_boot_degraded=1;
        log_simple("ERROR","recovery","mapping_failed","Recovery dm-verity mapping could not be activated");
    }
    int recovery_ok=recovery_map_ok && nova_mount(NOVA_RECOVERY_DM_NODE,"/Recovery","ext4",NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC,(void*)0)>=0;
    if(recovery_map_ok && !recovery_ok){
        g_recovery_verity=VERITY_FAILED; g_boot_degraded=1;
        log_simple("ERROR","recovery","mount_rejected","dm-verity rejected Recovery blocks; trusted recovery is unavailable");
    }

    g_storage_ready=data_persistent && system_ok && recovery_ok && g_data_writable &&
                    g_verity==VERITY_ENFORCED && g_recovery_verity==VERITY_ENFORCED;
    if(!g_storage_ready){
        g_boot_degraded=1;
        log_simple("ERROR","storage","mount_failed","normal verified System + Recovery + persistent Data policy could not be established; diagnostic Recovery may still be available independently");
    } else {
        log_simple("INFO","verity","mapping_active","active System slot is mounted through dm-verity");
        log_simple("INFO","recovery","mapping_active","Recovery is mounted through the firmware-bound signed dm-verity descriptor");
        log_simple("INFO","storage","persistent_ready","GPT verified System A/B + verified Recovery + persistent Data mounted under TPM anti-rollback policy");
    }
}

static int release_id_revoked(const char *id) {
    return release_list_contains(g_release_revoked_key_ids,id);
}

static int load_release_policy(void) {
    g_release_policy_verified=0; g_release_key=-1; g_release_next_key=-1; g_release_trust_epoch=0; g_trust_epoch_rejected=0;
    g_release_policy_format=0; g_release_policy_sha256[0]='\0';
    copy_string(g_release_policy_state,sizeof(g_release_policy_state),"unavailable");
    copy_string(g_release_active_key_id,sizeof(g_release_active_key_id),"none");
    copy_string(g_release_next_key_id,sizeof(g_release_next_key_id),"none");
    copy_string(g_release_revoked_key_ids,sizeof(g_release_revoked_key_ids),"none");
    g_release_active_cert_sha256[0]='\0'; g_release_next_cert_sha256[0]='\0';
    if(g_trust!=TRUST_VERIFIED || g_integrity!=INTEGRITY_VERIFIED || g_tpm_state!=TPM_STATE_READY) return 0;

    struct nova_release_policy_view policy;
    if(!verify_release_policy_bundle(NOVA_RELEASE_POLICY,NOVA_RELEASE_POLICY_SIGNATURE,
                                     NOVA_RELEASE_CERT,NOVA_RELEASE_NEXT_CERT,1,&policy)){
        struct nova_release_policy_view parsed;
        if(verify_signature_with_loaded_root(NOVA_RELEASE_POLICY,NOVA_RELEASE_POLICY_SIGNATURE) &&
           parse_release_policy_file(NOVA_RELEASE_POLICY,&parsed)){
            /* Preserve authenticated policy metadata for diagnostics/evidence
             * without authorizing its delegated keys.  parsed.active_key/next_key
             * remain -1 and release_policy_verified stays false. */
            (void)nova_sha256_file(NOVA_RELEASE_POLICY,parsed.policy_sha256);
            apply_release_policy_view(&parsed);
            if(!release_policy_epoch_allowed(parsed.trust_epoch)) g_trust_epoch_rejected=1;
        }
        return 0;
    }
    /* Delegated-update slots are authorized before /System mount using the
     * slot-scoped root-signed policy sidecar.  The mounted System must carry
     * byte-identical policy content or the boot fails closed. */
    if(g_active_slot_descriptor_format==5){
        if(!g_preboot_release_policy_verified || !nova_streq(g_preboot_release_policy_sha256,policy.policy_sha256)){
            copy_string(g_evidence_error,sizeof(g_evidence_error),"preboot-policy-bind-mismatch");
            return 0;
        }
    }
    apply_release_policy_view(&policy);
    g_release_policy_verified=1;
    log_simple("INFO","trust","release_policy_verified","root-signed delegated release policy accepted and bound to the mounted System");
    return 1;
}

struct nova_update_descriptor {
    struct nova_verity_descriptor verity;
    unsigned long trust_epoch;
    char signer_key_id[64];
    char transition[24];
    char target_policy_sha256[65];
};

static int verify_signature_with_key(long key_id, const char *manifest_path, const char *signature_path) {
    if(key_id<0) return 0;
    unsigned char signature[SIGNATURE_BUFFER], digest[32];
    long sig_len=read_binary_file(signature_path,signature,sizeof(signature));
    if(sig_len<=0 || nova_sha256_file_digest(manifest_path,digest)<0) return 0;
    struct nova_keyctl_pkey_params params; nova_zero(&params,sizeof(params));
    params.key_id=(int)key_id; params.in_len=32; params.in2_len=(unsigned int)sig_len;
    const char info[]="enc=pkcs1 hash=sha256";
    return nova_keyctl_pkey_verify(&params,info,digest,signature)>=0;
}

static int verify_signature_with_loaded_root(const char *manifest_path, const char *signature_path) {
    return verify_signature_with_key(g_trust_key,manifest_path,signature_path);
}

static int parse_update_descriptor(struct nova_update_descriptor *out) {
    nova_zero(out,sizeof(*out));
    struct nova_update_auth_fields auth;
    if(!parse_update_manifest_v5(NOVA_UPDATE_MANIFEST,&out->verity,&auth)) return 0;
    out->trust_epoch=auth.trust_epoch;
    copy_string(out->signer_key_id,sizeof(out->signer_key_id),auth.signer_key_id);
    copy_string(out->transition,sizeof(out->transition),auth.transition);
    copy_string(out->target_policy_sha256,sizeof(out->target_policy_sha256),auth.target_policy_sha256);
    return 1;
}

static int verify_update_bundle(struct nova_update_descriptor *desc, int verbose) {
    g_update_bundle_verified=0;
    if(!g_storage_persistent || g_trust!=TRUST_VERIFIED || g_integrity!=INTEGRITY_VERIFIED){if(verbose)nova_putln("update: persistent trusted system required");return 0;}
    if(!g_release_policy_verified || g_release_key<0){if(verbose)nova_putln("update: delegated release policy unavailable");return 0;}
    if(!parse_update_descriptor(desc)){if(verbose)nova_putln("update: descriptor invalid");return 0;}
    if(release_id_revoked(desc->signer_key_id)){if(verbose)nova_putln("update: revoked release signer rejected");return 0;}

    long signer_key=-1; int rotation=0;
    if(nova_streq(desc->signer_key_id,g_release_active_key_id)){
        if(desc->trust_epoch!=g_release_trust_epoch || !nova_streq(desc->transition,"none")){
            if(verbose)nova_putln("update: active signer may only issue same-epoch non-rotation updates"); return 0;
        }
        signer_key=g_release_key;
    } else if(!nova_streq(g_release_next_key_id,"none") && nova_streq(desc->signer_key_id,g_release_next_key_id)){
        unsigned long next_epoch=0;
        if(g_release_next_key<0 || !checked_add_ul(g_release_trust_epoch,1UL,&next_epoch) || desc->trust_epoch!=next_epoch || !nova_streq(desc->transition,"promote-next")){
            if(verbose)nova_putln("update: next signer requires an exact +1 promote-next transition"); return 0;
        }
        signer_key=g_release_next_key; rotation=1;
    } else { if(verbose)nova_putln("update: unknown release signer rejected"); return 0; }
    if(!verify_signature_with_key(signer_key,NOVA_UPDATE_MANIFEST,NOVA_UPDATE_SIGNATURE)){if(verbose)nova_putln("update: delegated release signature rejected");return 0;}

    char actual[65]; if(nova_sha256_file(NOVA_UPDATE_PAYLOAD,actual)<0 || !nova_streq(actual,desc->verity.payload_hash)){
        if(verbose)nova_putln("update: payload SHA-256 mismatch"); return 0;
    }

    struct nova_release_policy_view target;
    if(!verify_release_policy_bundle(NOVA_UPDATE_RELEASE_POLICY,NOVA_UPDATE_RELEASE_POLICY_SIGNATURE,
                                     NOVA_UPDATE_RELEASE_CERT,NOVA_UPDATE_RELEASE_NEXT_CERT,1,&target)){
        if(verbose)nova_putln("update: target release policy sidecar rejected"); return 0;
    }
    if(!nova_streq(desc->target_policy_sha256,target.policy_sha256) || target.trust_epoch!=desc->trust_epoch ||
       !nova_streq(target.active_key_id,desc->signer_key_id)){
        if(verbose)nova_putln("update: target policy does not match signed transition metadata"); return 0;
    }
    if(!rotation){
        if(!nova_streq(target.policy_sha256,g_release_policy_sha256)){
            if(verbose)nova_putln("update: same-epoch update attempted to mutate release trust policy"); return 0;
        }
    } else {
        if(!nova_streq(target.active_cert_sha256,g_release_next_cert_sha256)){
            if(verbose)nova_putln("update: rotation target active certificate is not the exact previously delegated next key"); return 0;
        }
        if(!release_list_contains(target.revoked_key_ids,g_release_active_key_id)){
            if(verbose)nova_putln("update: rotation target did not revoke the previous active release key"); return 0;
        }
        if(!release_revocation_superset(target.revoked_key_ids,g_release_revoked_key_ids)){
            if(verbose)nova_putln("update: rotation target attempted to remove a previously revoked release key"); return 0;
        }
    }

    g_update_bundle_verified=1; copy_string(g_update_bundle_id,sizeof(g_update_bundle_id),desc->verity.id);
    copy_string(g_update_bundle_version,sizeof(g_update_bundle_version),desc->verity.version);
    copy_string(g_update_payload_sha256,sizeof(g_update_payload_sha256),desc->verity.payload_hash);
    if(verbose){nova_puts("update: VERIFIED id=");nova_puts(desc->verity.id);nova_puts(" version=");nova_puts(desc->verity.version);
        nova_puts(" rollback_index=");nova_put_u64(desc->verity.rollback_index);nova_puts(" trust_epoch=");nova_put_u64(desc->trust_epoch);
        nova_puts(" signer=");nova_puts(desc->signer_key_id);nova_puts(" transition=");nova_puts(desc->transition);
        nova_puts(" target_policy=");nova_puts(desc->target_policy_sha256);nova_puts(" verity_root=");nova_putln(desc->verity.root_hash);}
    return 1;
}

static int copy_file_atomic(const char *src, const char *tmp, const char *dst) {
    long in=nova_open_ro(src); if(in<0) return 0;
    long out=nova_open_write(tmp,0644); if(out<0){nova_close((int)in);return 0;}
    unsigned char buffer[FILE_BUFFER]; int ok=1;
    for(;;){
        long n=nova_read((int)in,buffer,sizeof(buffer));
        if(n==-NOVA_EINTR) continue;
        if(n<0){ok=0;break;} if(n==0) break;
        if(nova_fd_write_all((int)out,buffer,(nova_size_t)n)<0){ok=0;break;}
    }
    if(ok && nova_fsync((int)out)<0) ok=0;
    nova_close((int)in); nova_close((int)out);
    if(!ok){(void)nova_unlink(tmp);return 0;}
    if(nova_rename(tmp,dst)<0){(void)nova_unlink(tmp);return 0;}
    nova_sync(); return 1;
}

static int copy_file_atomic_path(const char *src, const char *dst) {
    char tmp[192]; copy_string(tmp,sizeof(tmp),dst); nova_size_t n=nova_strlen(tmp);
    if(n+4>=sizeof(tmp)) return 0;
    tmp[n++]='.';tmp[n++]='n';tmp[n++]='e';tmp[n++]='w';tmp[n]='\0';
    return copy_file_atomic(src,tmp,dst);
}

static const char *slot_trust_dir(char slot){return slot=='B'?NOVA_SLOT_B_TRUST_DIR:NOVA_SLOT_A_TRUST_DIR;}

static int install_update_slot_descriptor(char target, const struct nova_update_descriptor *desc) {
    if(!desc || desc->verity.format!=5) return 0;
    struct nova_release_policy_view target_policy;
    if(!verify_release_policy_bundle(NOVA_UPDATE_RELEASE_POLICY,NOVA_UPDATE_RELEASE_POLICY_SIGNATURE,
                                     NOVA_UPDATE_RELEASE_CERT,NOVA_UPDATE_RELEASE_NEXT_CERT,1,&target_policy)) return 0;
    if(!nova_streq(target_policy.policy_sha256,desc->target_policy_sha256) ||
       target_policy.trust_epoch!=desc->trust_epoch || !nova_streq(target_policy.active_key_id,desc->signer_key_id)) return 0;

    ensure_dir(slot_trust_dir(target));
    if(!copy_file_atomic_path(NOVA_UPDATE_RELEASE_POLICY,slot_release_policy(target))) return 0;
    if(!copy_file_atomic_path(NOVA_UPDATE_RELEASE_POLICY_SIGNATURE,slot_release_policy_signature(target))) return 0;
    if(!copy_file_atomic_path(NOVA_UPDATE_RELEASE_CERT,slot_release_cert(target))) return 0;
    if(!nova_streq(target_policy.next_key_id,"none")){
        if(!copy_file_atomic_path(NOVA_UPDATE_RELEASE_NEXT_CERT,slot_release_next_cert(target))) return 0;
    } else (void)nova_unlink(slot_release_next_cert(target));

    /* Descriptor/signature are installed last.  A power loss before the slot
     * journal commit can therefore only damage the inactive slot; it cannot
     * authorize a boot transition. */
    if(!copy_file_atomic_path(NOVA_UPDATE_MANIFEST,slot_verity_manifest(target))) return 0;
    if(!copy_file_atomic_path(NOVA_UPDATE_SIGNATURE,slot_verity_signature(target))) return 0;
    struct nova_verity_descriptor check;
    if(!verify_slot_verity_descriptor_ex(target,&check,0)) return 0;
    return check.format==desc->verity.format && check.rollback_index==desc->verity.rollback_index &&
           nova_streq(check.id,desc->verity.id) && nova_streq(check.payload_hash,desc->verity.payload_hash) &&
           nova_streq(check.root_hash,desc->verity.root_hash);
}

static int copy_update_payload_to_device(const char *device, const struct nova_update_descriptor *desc) {
    long in=nova_open_ro(NOVA_UPDATE_PAYLOAD); if(in<0) return 0;
    long out=nova_open_wo(device); if(out<0){nova_close((int)in);return 0;}
    unsigned char buffer[FILE_BUFFER]; unsigned long total=0;
    for(;;){
        long n=nova_read((int)in,buffer,sizeof(buffer));
        if(n==-NOVA_EINTR) continue;
        if(n<0){nova_close((int)in);nova_close((int)out);return 0;}
        if(n==0) break;
        if(total+(unsigned long)n>NOVA_SYSTEM_SLOT_BYTES || nova_fd_write_all((int)out,buffer,(nova_size_t)n)<0){nova_close((int)in);nova_close((int)out);return 0;}
        total+=(unsigned long)n;
    }
    int ok=(total==desc->verity.payload_size && nova_fsync((int)out)>=0);
    nova_close((int)in); nova_close((int)out); nova_sync();
    if(!ok) return 0;
    char raw_hash[65]; return nova_sha256_file(device,raw_hash)>=0 && nova_streq(raw_hash,desc->verity.payload_hash);
}

static const char *inactive_system_device(void) { return g_active_slot=='A'?NOVA_SYSTEM_B_DEV:NOVA_SYSTEM_A_DEV; }
static char inactive_slot(void) { return g_active_slot=='A'?'B':'A'; }

static void cmd_update_status(void) {
    nova_putln("NovaOS Stage-4D delegated-release update state");
    nova_puts("  Bundle dir      : "); nova_putln(NOVA_UPDATE_DIR);
    nova_puts("  Running slot    : "); char a[2]={g_active_slot,'\0'}; nova_putln(a);
    nova_puts("  Inactive slot   : "); char b[2]={inactive_slot(),'\0'}; nova_putln(b);
    nova_puts("  Pending kind    : "); nova_putln(g_pending_kind);
    nova_puts("  Update ID       : "); nova_putln(g_update_id);
    nova_puts("  Attempts left   : "); nova_put_u64(g_boot_attempts_left); nova_putln("");
    nova_puts("  Bundle verified : "); nova_putln(g_update_bundle_verified?"yes":"not checked");
    nova_puts("  TPM counter     : "); nova_put_u64(g_tpm_rollback_counter); nova_putln("");
    nova_puts("  Active rollback : "); nova_put_u64(g_active_rollback_index); nova_putln("");
    nova_putln("  Update trust     : authenticated delegated release signer + signed verity descriptor + full slot SHA-256");
    nova_putln("  Anti-rollback    : signed update rollback_index must equal TPM counter + 1");
    nova_putln("  dm-verity        : update payload carries authenticated root hash + hash tree");
}

static int mutation_health_ready(void) {
    /* Persistent mutation is authorized by exactly the same predicate that can
     * authorize normal READY operation.  RECOVERY_REQUIRED therefore remains
     * diagnostic-only even when /Data itself is writable. */
    return normal_boot_policy_satisfied();
}

static void update_action(const char *args) {
    args=nova_skip_spaces(args);
    if(nova_streq(args,"status")){cmd_update_status();return;}
    if(nova_streq(args,"verify")){
        struct nova_update_descriptor desc; (void)verify_update_bundle(&desc,1); return;
    }
    if(nova_streq(args,"apply")){
        if(!mutation_health_ready()){nova_putln("update: refused because current firmware/System/Recovery/TPM measured-boot health is not fully verified");return;}
        if(!g_slot_confirmed){nova_putln("update: refused while another slot transition is pending");return;}
        struct nova_update_descriptor desc;
        if(!verify_update_bundle(&desc,1)) return;
        if(g_tpm_state!=TPM_STATE_READY){nova_putln("update: refused because TPM anti-rollback state is unavailable");return;}
        unsigned long expected_rollback=0;
        if(!checked_add_ul(g_tpm_rollback_counter,1UL,&expected_rollback) || desc.verity.rollback_index!=expected_rollback){
            nova_puts("update: refused rollback_index=");nova_put_u64(desc.verity.rollback_index);
            nova_puts(" expected=");
            if(expected_rollback) nova_put_u64(expected_rollback); else nova_puts("counter-exhausted");
            nova_putln(""); return;
        }
        if(desc.verity.rollback_index<=g_active_rollback_index){nova_putln("update: refused because rollback index does not advance the running System");return;}
        char target=inactive_slot(); const char *device=inactive_system_device();
        nova_puts("update: writing signed payload to inactive System "); char t[2]={target,'\0'}; nova_putln(t);
        if(!copy_update_payload_to_device(device,&desc)){nova_putln("update: inactive-slot write verification FAILED");log_simple("ERROR","update","slot_write_failed","inactive System slot did not match signed update payload");return;}
        if(!install_update_slot_descriptor(target,&desc)){nova_putln("update: signed verity descriptor install FAILED");log_simple("ERROR","update","verity_descriptor_install_failed","inactive slot verity descriptor could not be authenticated after install");return;}
        unsigned long next_generation=0;
        if(!checked_add_ul(g_slot_generation,1UL,&next_generation) ||
           !commit_slot_metadata(target,g_active_slot,next_generation,0,1,"update",desc.verity.id)){
            nova_putln("update: slot metadata generation exhausted or commit failed"); return;
        }
        g_previous_slot=g_active_slot; g_next_slot=target; g_slot_generation=next_generation; g_slot_confirmed=0; g_boot_attempts_left=1;
        copy_string(g_pending_kind,sizeof(g_pending_kind),"update"); copy_string(g_update_id,sizeof(g_update_id),desc.verity.id);
        write_boot_state();
        log_simple("INFO","update","staged","signed verity-protected update written to inactive slot and scheduled for next boot");
        nova_puts("update: STAGED id=");nova_puts(desc.verity.id);nova_puts(" next_slot=");nova_putln(t);
        nova_putln("update: one unconfirmed boot attempt is allowed; next reboot rolls back automatically unless slot confirm succeeds");
        return;
    }
    nova_putln("usage: update <status|verify|apply>");
}

static int recover_tpm_counter_commit_if_needed(void) {
    if(g_tpm_state!=TPM_STATE_READY) return 0;
    if(!g_slot_confirmed) return 1;
    if(g_firmware_trust!=FIRMWARE_TRUST_VERIFIED || !g_storage_ready || g_verity!=VERITY_ENFORCED ||
       g_recovery_verity!=VERITY_ENFORCED || g_trust!=TRUST_VERIFIED || g_integrity!=INTEGRITY_VERIFIED || !g_release_policy_verified) return 0;
    unsigned long pending_rollback=0,committed_epoch=0,pending_epoch=0;
    if(!checked_add_ul(g_tpm_rollback_counter,1UL,&pending_rollback) ||
       g_active_rollback_index<g_tpm_rollback_counter || g_active_rollback_index>pending_rollback) return 0;
    if(!release_epoch_bounds(&committed_epoch,&pending_epoch) ||
       g_release_trust_epoch<committed_epoch || g_release_trust_epoch>pending_epoch) return 0;
    if(g_active_rollback_index==pending_rollback){
        unsigned long new_value=0;
        if(!nova_tpm2_increment_rollback_counter(g_tpm_rollback_counter,&new_value) || new_value!=g_active_rollback_index){
            g_tpm_state=TPM_STATE_FAILED; g_boot_degraded=1;
            log_simple("ERROR","tpm","counter_commit_recovery_failed","confirmed System is ahead of TPM rollback counter and recovery commit failed");
            return 0;
        }
        g_tpm_rollback_counter=new_value; g_tpm_counter_commit_recovered=1;
        log_simple("WARN","tpm","counter_commit_recovered","recovered interrupted confirmation by advancing TPM rollback counter");
    }
    if(!release_epoch_bounds(&committed_epoch,&pending_epoch)) return 0;
    if(g_release_trust_epoch==pending_epoch){
        unsigned long new_value=0,new_committed_epoch=0;
        if(!nova_tpm2_increment_trust_epoch_counter(g_tpm_trust_epoch_counter,&new_value) ||
           !checked_add_ul(new_value,1UL,&new_committed_epoch) || new_committed_epoch!=g_release_trust_epoch){
            g_tpm_state=TPM_STATE_FAILED; g_boot_degraded=1;
            log_simple("ERROR","tpm","trust_epoch_commit_recovery_failed","confirmed release policy is ahead of TPM trust epoch and recovery commit failed");
            return 0;
        }
        g_tpm_trust_epoch_counter=new_value; g_tpm_trust_epoch_commit_recovered=1;
        log_simple("WARN","tpm","trust_epoch_commit_recovered","recovered interrupted release-key rotation by advancing TPM trust epoch counter");
    }
    return 1;
}

static int perform_measured_boot(void) {
    if(g_measured_boot){copy_string(g_evidence_error,sizeof(g_evidence_error),"none");return 1;}
    if(g_tpm_state!=TPM_STATE_READY){copy_string(g_evidence_error,sizeof(g_evidence_error),"tpm-unavailable");return 0;}
    if(g_firmware_trust!=FIRMWARE_TRUST_VERIFIED){copy_string(g_evidence_error,sizeof(g_evidence_error),"firmware-trust-unverified");return 0;}
    if(g_antirollback_rejected){copy_string(g_evidence_error,sizeof(g_evidence_error),"rollback-generation-rejected");return 0;}
    if(g_trust_epoch_rejected){copy_string(g_evidence_error,sizeof(g_evidence_error),"trust-epoch-rejected");return 0;}
    if(g_verity!=VERITY_ENFORCED){copy_string(g_evidence_error,sizeof(g_evidence_error),"system-verity-unverified");return 0;}
    if(g_recovery_verity!=VERITY_ENFORCED){copy_string(g_evidence_error,sizeof(g_evidence_error),"recovery-verity-unverified");return 0;}
    if(g_trust!=TRUST_VERIFIED || g_integrity!=INTEGRITY_VERIFIED){copy_string(g_evidence_error,sizeof(g_evidence_error),"content-trust-unverified");return 0;}
    if(!g_release_policy_verified){if(nova_streq(g_evidence_error,"not-run"))copy_string(g_evidence_error,sizeof(g_evidence_error),"release-policy-unverified");return 0;}
    if(!nova_tpm2_read_pcr_sha256(7,g_pcr7_sha256)){
        g_measurement_failed=1; g_boot_degraded=1;
        copy_string(g_pcr7_source,sizeof(g_pcr7_source),nova_tpm2_last_pcr_source());
        copy_string(g_evidence_error,sizeof(g_evidence_error),"pcr7-read-failed");
        log_simple("ERROR","tpm","pcr7_read_failed","TPM SHA-256 PCR 7 could not be read for measured boot; NV anti-rollback state remains separately tracked");
        return 0;
    }
    copy_string(g_pcr7_source,sizeof(g_pcr7_source),nova_tpm2_last_pcr_source());
    struct nova_tcg2_evidence evidence;
    int event_log_ok=nova_tcg2_collect_evidence(NOVA_TCG_EVENT_LOG,&evidence);
    /* Copy diagnostic fields even for rejected logs. A raw digest/byte count and
     * any fully parsed prefix are useful forensic evidence, but never authorize
     * boot unless the full parser succeeds and replay matches live PCR7. */
    copy_string(g_event_log_sha256,sizeof(g_event_log_sha256),evidence.log_sha256);
    copy_string(g_event_log_replayed_pcr7,sizeof(g_event_log_replayed_pcr7),evidence.replayed_pcr7_sha256);
    g_event_log_bytes=evidence.log_bytes; g_event_log_events=evidence.event2_count; g_event_log_pcr7_events=evidence.pcr7_sha256_events;
    if(!event_log_ok){
        g_measurement_failed=1; g_boot_degraded=1; g_event_log_verified=0;
        copy_string(g_event_log_error,sizeof(g_event_log_error),nova_tcg2_last_error());
        copy_string(g_evidence_error,sizeof(g_evidence_error),"event-log-invalid");
        log_simple("ERROR","evidence","event_log_parse_failed","firmware TPM event log could not be parsed/replayed; measured boot evidence rejected");
        return 0;
    }
    if(!nova_streq(g_event_log_replayed_pcr7,g_pcr7_sha256)){
        g_measurement_failed=1; g_boot_degraded=1; g_event_log_verified=0;
        copy_string(g_event_log_error,sizeof(g_event_log_error),"pcr7-replay-mismatch");
        copy_string(g_evidence_error,sizeof(g_evidence_error),"pcr7-replay-mismatch");
        log_simple("ERROR","evidence","pcr7_replay_mismatch","firmware event-log replay does not match the live TPM PCR 7 value");
        return 0;
    }
    g_event_log_verified=1; copy_string(g_event_log_error,sizeof(g_event_log_error),"none");
    log_simple("INFO","evidence","event_log_verified","firmware TPM event log replay matched live SHA-256 PCR 7");
    if(!nova_tpm2_read_pcr_sha256(14,g_pcr14_before)){
        g_measurement_failed=1; g_boot_degraded=1;
        copy_string(g_pcr14_before_source,sizeof(g_pcr14_before_source),nova_tpm2_last_pcr_source());
        copy_string(g_evidence_error,sizeof(g_evidence_error),"pcr14-read-failed");
        log_simple("ERROR","tpm","pcr14_read_failed","TPM SHA-256 PCR 14 could not be read before measured-boot extend; NV anti-rollback state remains separately tracked");
        return 0;
    }
    copy_string(g_pcr14_before_source,sizeof(g_pcr14_before_source),nova_tpm2_last_pcr_source());
    /* Canonical Stage-4D measurement preimage v2.  This is deliberately
     * versioned independently from the product version, GPT layout, release
     * policy schema, and boot-evidence schema.  Every field below is already
     * authenticated or live-verified before this point; PCR14 binds their
     * exact boot-time values into TPM state rather than merely recording them
     * in a mutable /run file. */
    struct nova_sha256_ctx ctx; unsigned char digest[32];
    nova_sha256_init(&ctx);
    sha_update_string(&ctx,"NovaOS Stage-4D measured boot evidence v2\n");
    sha_update_string(&ctx,"evidence_schema=3\nversion="); sha_update_string(&ctx,NOVA_VERSION);
    sha_update_string(&ctx,"\nslot="); nova_sha256_update(&ctx,&g_active_slot,1);
    sha_update_string(&ctx,"\nslot_descriptor_format="); sha_update_u64_ascii(&ctx,(unsigned long)g_active_slot_descriptor_format);
    sha_update_string(&ctx,"\nslot_descriptor_kind="); sha_update_string(&ctx,g_active_slot_descriptor_kind);
    sha_update_string(&ctx,"\nrollback_index="); sha_update_u64_ascii(&ctx,g_active_rollback_index);
    sha_update_string(&ctx,"\ntpm_rollback_counter="); sha_update_u64_ascii(&ctx,g_tpm_rollback_counter);
    sha_update_string(&ctx,"\nsystem_root="); sha_update_string(&ctx,g_verity_root_hash);
    sha_update_string(&ctx,"\nrecovery_root="); sha_update_string(&ctx,g_recovery_verity_root_hash);
    sha_update_string(&ctx,"\nsecure_boot="); sha_update_string(&ctx,g_firmware_secure_boot?"enabled":"disabled");
    sha_update_string(&ctx,"\nfirmware_trust="); sha_update_string(&ctx,firmware_trust_name());
    sha_update_string(&ctx,"\npcr7="); sha_update_string(&ctx,g_pcr7_sha256);
    sha_update_string(&ctx,"\nevent_log_sha256="); sha_update_string(&ctx,g_event_log_sha256);
    sha_update_string(&ctx,"\nevent_log_replayed_pcr7="); sha_update_string(&ctx,g_event_log_replayed_pcr7);
    sha_update_string(&ctx,"\nrelease_policy_sha256="); sha_update_string(&ctx,g_release_policy_sha256);
    sha_update_string(&ctx,"\npreboot_release_policy_sha256=");
    sha_update_string(&ctx,g_preboot_release_policy_sha256[0]?g_preboot_release_policy_sha256:"unavailable");
    sha_update_string(&ctx,"\nrelease_active_cert_sha256="); sha_update_string(&ctx,g_release_active_cert_sha256);
    sha_update_string(&ctx,"\nrelease_key_id="); sha_update_string(&ctx,g_release_active_key_id);
    sha_update_string(&ctx,"\ntrust_epoch="); sha_update_u64_ascii(&ctx,g_release_trust_epoch);
    sha_update_string(&ctx,"\ntpm_trust_epoch_counter="); sha_update_u64_ascii(&ctx,g_tpm_trust_epoch_counter);
    sha_update_string(&ctx,"\nuefi_db_cert_sha256="); sha_update_string(&ctx,NOVA_UEFI_CERT_SHA256);
    sha_update_string(&ctx,"\n");
    nova_sha256_final(&ctx,digest); nova_sha256_hex(digest,g_measurement_digest);

    unsigned char old_pcr[32], expected_bytes[32]; char expected_hex[65];
    if(!hex32_to_bytes(g_pcr14_before,old_pcr)){
        g_measurement_failed=1; g_boot_degraded=1;
        copy_string(g_evidence_error,sizeof(g_evidence_error),"pcr14-decode-failed");
        log_simple("ERROR","tpm","pcr14_decode_failed","PCR 14 value was not a valid SHA-256 digest");
        return 0;
    }
    struct nova_sha256_ctx pctx; nova_sha256_init(&pctx); nova_sha256_update(&pctx,old_pcr,32); nova_sha256_update(&pctx,digest,32);
    nova_sha256_final(&pctx,expected_bytes); nova_sha256_hex(expected_bytes,expected_hex);
    if(!nova_tpm2_pcr_extend_sha256(14,digest)){
        g_measurement_failed=1; g_boot_degraded=1;
        copy_string(g_evidence_error,sizeof(g_evidence_error),"pcr14-extend-failed");
        log_simple("ERROR","tpm","pcr14_extend_failed","TPM2_PCR_Extend failed for PCR 14; NV anti-rollback state remains separately tracked");
        return 0;
    }
    if(!nova_tpm2_read_pcr_sha256(14,g_pcr14_after)){
        g_measurement_failed=1; g_boot_degraded=1;
        copy_string(g_pcr14_after_source,sizeof(g_pcr14_after_source),nova_tpm2_last_pcr_source());
        copy_string(g_evidence_error,sizeof(g_evidence_error),"pcr14-verify-read-failed");
        log_simple("ERROR","tpm","pcr14_verify_read_failed","PCR 14 could not be re-read after measured-boot extend");
        return 0;
    }
    copy_string(g_pcr14_after_source,sizeof(g_pcr14_after_source),nova_tpm2_last_pcr_source());
    if(!nova_streq(expected_hex,g_pcr14_after)){
        g_measurement_failed=1; g_boot_degraded=1;
        copy_string(g_evidence_error,sizeof(g_evidence_error),"pcr14-extend-mismatch");
        log_simple("ERROR","tpm","pcr14_extend_mismatch","PCR 14 did not match the expected NovaOS measured-boot extension");
        return 0;
    }
    long fd=nova_open_write(NOVA_MEASUREMENTS_LOG,0644);
    if(fd>=0){
        nova_fd_putln((int)fd,"# NovaOS Stage-4D runtime measurement event");
        nova_fd_putln((int)fd,"measurement_format=2");
        nova_fd_putln((int)fd,"evidence_schema=3");
        nova_fd_puts((int)fd,"version="); nova_fd_putln((int)fd,NOVA_VERSION);
        nova_fd_putln((int)fd,"pcr=14");
        nova_fd_putln((int)fd,"algorithm=sha256");
        nova_fd_puts((int)fd,"slot="); char slot[2]={g_active_slot,'\0'}; nova_fd_putln((int)fd,slot);
        nova_fd_puts((int)fd,"slot_descriptor_format="); nova_fd_put_u64((int)fd,(unsigned long)g_active_slot_descriptor_format); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"slot_descriptor_kind="); nova_fd_putln((int)fd,g_active_slot_descriptor_kind);
        nova_fd_puts((int)fd,"rollback_index="); nova_fd_put_u64((int)fd,g_active_rollback_index); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"tpm_rollback_counter="); nova_fd_put_u64((int)fd,g_tpm_rollback_counter); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"system_root="); nova_fd_putln((int)fd,g_verity_root_hash);
        nova_fd_puts((int)fd,"recovery_root="); nova_fd_putln((int)fd,g_recovery_verity_root_hash);
        nova_fd_puts((int)fd,"secure_boot="); nova_fd_putln((int)fd,g_firmware_secure_boot?"enabled":"disabled");
        nova_fd_puts((int)fd,"firmware_trust="); nova_fd_putln((int)fd,firmware_trust_name());
        nova_fd_puts((int)fd,"pcr7="); nova_fd_putln((int)fd,g_pcr7_sha256);
        nova_fd_puts((int)fd,"event_log_sha256="); nova_fd_putln((int)fd,g_event_log_sha256);
        nova_fd_puts((int)fd,"event_log_replayed_pcr7="); nova_fd_putln((int)fd,g_event_log_replayed_pcr7);
        nova_fd_puts((int)fd,"release_policy_sha256="); nova_fd_putln((int)fd,g_release_policy_sha256);
        nova_fd_puts((int)fd,"preboot_release_policy_sha256="); nova_fd_putln((int)fd,g_preboot_release_policy_sha256[0]?g_preboot_release_policy_sha256:"unavailable");
        nova_fd_puts((int)fd,"release_active_cert_sha256="); nova_fd_putln((int)fd,g_release_active_cert_sha256);
        nova_fd_puts((int)fd,"release_key_id="); nova_fd_putln((int)fd,g_release_active_key_id);
        nova_fd_puts((int)fd,"trust_epoch="); nova_fd_put_u64((int)fd,g_release_trust_epoch); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"tpm_trust_epoch_counter="); nova_fd_put_u64((int)fd,g_tpm_trust_epoch_counter); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"uefi_db_cert_sha256="); nova_fd_putln((int)fd,NOVA_UEFI_CERT_SHA256);
        nova_fd_puts((int)fd,"pcr14_before="); nova_fd_putln((int)fd,g_pcr14_before);
        nova_fd_puts((int)fd,"event_digest="); nova_fd_putln((int)fd,g_measurement_digest);
        nova_fd_puts((int)fd,"pcr14_after="); nova_fd_putln((int)fd,g_pcr14_after);
        nova_close((int)fd);
    }
    g_measurement_failed=0; g_measured_boot=1;
    copy_string(g_evidence_error,sizeof(g_evidence_error),"none");
    log_simple("INFO","tpm","measured_boot_extended","verified NovaOS boot evidence extended into TPM SHA-256 PCR 14");
    return 1;
}

static int normal_boot_policy_satisfied(void) {
    return g_firmware_trust==FIRMWARE_TRUST_VERIFIED && g_firmware_secure_boot &&
           g_tpm_state==TPM_STATE_READY && g_measured_boot && g_event_log_verified &&
           !g_antirollback_rejected && !g_trust_epoch_rejected && !g_slot_journal_write_failed &&
           g_storage_ready && g_verity==VERITY_ENFORCED && g_recovery_verity==VERITY_ENFORCED &&
           g_trust==TRUST_VERIFIED && g_integrity==INTEGRITY_VERIFIED && g_release_policy_verified &&
           g_system_readonly && g_recovery_readonly;
}

static int boot_evidence_verified(void) {
    return normal_boot_policy_satisfied();
}

static void write_boot_evidence(void) {
    if(nova_getpid()!=1) return;
    long fd=nova_open_write(NOVA_EVIDENCE_FILE,0644); if(fd<0) return;
    const int verified=boot_evidence_verified();
    if(!verified && (nova_streq(g_evidence_error,"not-run") || nova_streq(g_evidence_error,"none"))){
        if(g_firmware_trust!=FIRMWARE_TRUST_VERIFIED) copy_string(g_evidence_error,sizeof(g_evidence_error),"firmware-trust-unverified");
        else if(g_tpm_state!=TPM_STATE_READY) copy_string(g_evidence_error,sizeof(g_evidence_error),"tpm-unavailable");
        else if(g_antirollback_rejected) copy_string(g_evidence_error,sizeof(g_evidence_error),"rollback-generation-rejected");
        else if(g_trust_epoch_rejected) copy_string(g_evidence_error,sizeof(g_evidence_error),"trust-epoch-rejected");
        else if(g_slot_journal_write_failed) copy_string(g_evidence_error,sizeof(g_evidence_error),"slot-journal-write-failed");
        else if(!g_storage_ready) copy_string(g_evidence_error,sizeof(g_evidence_error),"verified-storage-unavailable");
        else if(g_trust!=TRUST_VERIFIED || g_integrity!=INTEGRITY_VERIFIED) copy_string(g_evidence_error,sizeof(g_evidence_error),"content-trust-unverified");
        else if(!g_release_policy_verified) copy_string(g_evidence_error,sizeof(g_evidence_error),"release-policy-unverified");
        else if(!g_event_log_verified) copy_string(g_evidence_error,sizeof(g_evidence_error),"event-log-unverified");
        else if(!g_measured_boot) copy_string(g_evidence_error,sizeof(g_evidence_error),"measured-boot-unverified");
        else copy_string(g_evidence_error,sizeof(g_evidence_error),"boot-evidence-unverified");
    }
    nova_fd_putln((int)fd,"schema=3");
    nova_fd_putln((int)fd,"evidence_kind=novaos-boot-evidence");
    nova_fd_puts((int)fd,"evidence_status=");nova_fd_putln((int)fd,verified?"VERIFIED":"REJECTED");
    nova_fd_puts((int)fd,"evidence_error=");nova_fd_putln((int)fd,verified?"none":g_evidence_error);
    nova_fd_puts((int)fd,"version=");nova_fd_putln((int)fd,NOVA_VERSION);
    nova_fd_puts((int)fd,"boot_stage=");nova_fd_putln((int)fd,g_boot_stage);
    nova_fd_puts((int)fd,"normal_boot_authorized=");nova_fd_putln((int)fd,nova_streq(g_boot_stage,"READY")?"yes":"no");
    nova_fd_puts((int)fd,"slot=");char slot[2]={g_active_slot,'\0'};nova_fd_putln((int)fd,slot);
    nova_fd_puts((int)fd,"slot_descriptor_format=");nova_fd_put_u64((int)fd,(unsigned long)g_active_slot_descriptor_format);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"slot_descriptor_kind=");nova_fd_putln((int)fd,g_active_slot_descriptor_kind);
    nova_fd_puts((int)fd,"rollback_index=");nova_fd_put_u64((int)fd,g_active_rollback_index);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"tpm_rollback_counter=");nova_fd_put_u64((int)fd,g_tpm_rollback_counter);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"trust_epoch=");nova_fd_put_u64((int)fd,g_release_trust_epoch);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"trust_epoch_state=");nova_fd_putln((int)fd,release_epoch_state(g_release_trust_epoch));
    nova_fd_puts((int)fd,"tpm_trust_epoch_counter=");nova_fd_put_u64((int)fd,g_tpm_trust_epoch_counter);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"secure_boot=");nova_fd_putln((int)fd,g_firmware_secure_boot?"enabled":"disabled");
    nova_fd_puts((int)fd,"firmware_trust=");nova_fd_putln((int)fd,firmware_trust_name());
    nova_fd_puts((int)fd,"release_policy_verified=");nova_fd_putln((int)fd,g_release_policy_verified?"yes":"no");
    nova_fd_puts((int)fd,"release_policy_format=");nova_fd_put_u64((int)fd,(unsigned long)g_release_policy_format);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"release_policy_state=");nova_fd_putln((int)fd,g_release_policy_state);
    nova_fd_puts((int)fd,"release_policy_sha256=");nova_fd_putln((int)fd,g_release_policy_sha256[0]?g_release_policy_sha256:"unavailable");
    nova_fd_puts((int)fd,"preboot_release_policy_verified=");nova_fd_putln((int)fd,g_preboot_release_policy_verified?"yes":"no");
    nova_fd_puts((int)fd,"preboot_release_policy_sha256=");nova_fd_putln((int)fd,g_preboot_release_policy_sha256[0]?g_preboot_release_policy_sha256:"unavailable");
    nova_fd_puts((int)fd,"release_key_id=");nova_fd_putln((int)fd,g_release_active_key_id);
    nova_fd_puts((int)fd,"release_active_cert_sha256=");nova_fd_putln((int)fd,g_release_active_cert_sha256[0]?g_release_active_cert_sha256:"unavailable");
    nova_fd_puts((int)fd,"release_next_key_id=");nova_fd_putln((int)fd,g_release_next_key_id);
    nova_fd_puts((int)fd,"release_next_cert_sha256=");nova_fd_putln((int)fd,g_release_next_cert_sha256[0]?g_release_next_cert_sha256:"unavailable");
    nova_fd_puts((int)fd,"release_revoked_key_ids=");nova_fd_putln((int)fd,g_release_revoked_key_ids);
    nova_fd_puts((int)fd,"system_root=");nova_fd_putln((int)fd,g_verity_root_hash[0]?g_verity_root_hash:"unavailable");
    nova_fd_puts((int)fd,"recovery_root=");nova_fd_putln((int)fd,g_recovery_verity_root_hash[0]?g_recovery_verity_root_hash:"unavailable");
    nova_fd_puts((int)fd,"event_log_sha256=");nova_fd_putln((int)fd,g_event_log_sha256[0]?g_event_log_sha256:"unavailable");
    nova_fd_puts((int)fd,"event_log_bytes=");nova_fd_put_u64((int)fd,g_event_log_bytes);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"event_log_events=");nova_fd_put_u64((int)fd,g_event_log_events);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"event_log_pcr7_events=");nova_fd_put_u64((int)fd,g_event_log_pcr7_events);nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"event_log_error=");nova_fd_putln((int)fd,g_event_log_error);
    nova_fd_puts((int)fd,"event_log_replay=");nova_fd_putln((int)fd,g_event_log_verified?"VERIFIED":"REJECTED");
    nova_fd_puts((int)fd,"pcr7_live=");nova_fd_putln((int)fd,g_pcr7_sha256[0]?g_pcr7_sha256:"unavailable");
    nova_fd_puts((int)fd,"pcr7_source=");nova_fd_putln((int)fd,g_pcr7_source);
    nova_fd_puts((int)fd,"pcr7_replayed=");nova_fd_putln((int)fd,g_event_log_replayed_pcr7[0]?g_event_log_replayed_pcr7:"unavailable");
    nova_fd_puts((int)fd,"pcr14_before=");nova_fd_putln((int)fd,g_pcr14_before[0]?g_pcr14_before:"unavailable");
    nova_fd_puts((int)fd,"pcr14_before_source=");nova_fd_putln((int)fd,g_pcr14_before_source);
    nova_fd_puts((int)fd,"event_digest=");nova_fd_putln((int)fd,g_measurement_digest[0]?g_measurement_digest:"unavailable");
    nova_fd_puts((int)fd,"pcr14_after=");nova_fd_putln((int)fd,g_pcr14_after[0]?g_pcr14_after:"unavailable");
    nova_fd_puts((int)fd,"pcr14_after_source=");nova_fd_putln((int)fd,g_pcr14_after_source);
    nova_close((int)fd);
}

static void copy_service(struct nova_service *dst, const struct nova_service *src) {
    copy_string(dst->name, sizeof(dst->name), src->name); copy_string(dst->exec, sizeof(dst->exec), src->exec);
    copy_string(dst->app_id, sizeof(dst->app_id), src->app_id);
    copy_string(dst->display_name, sizeof(dst->display_name), src->display_name);
    copy_string(dst->description, sizeof(dst->description), src->description);
    copy_string(dst->icon_asset_id, sizeof(dst->icon_asset_id), src->icon_asset_id);
    dst->client_id=src->client_id; dst->category=src->category;
    dst->declared_capabilities=src->declared_capabilities; dst->effective_capabilities=src->effective_capabilities;
    dst->files_user_home=src->files_user_home; dst->sandbox_profile=src->sandbox_profile; dst->sandbox_required_flags=src->sandbox_required_flags;
    dst->enabled=src->enabled; dst->critical=src->critical; dst->restart_always=src->restart_always;
    dst->autostart=src->autostart; dst->is_application=src->is_application;
    dst->desired_running=src->desired_running; dst->pid=src->pid; dst->restart_count=src->restart_count;
    dst->app_state=src->app_state; dst->app_generation=src->app_generation;
    dst->stop_deadline_ns=src->stop_deadline_ns; dst->stop_fallback_ticks=src->stop_fallback_ticks;
    dst->stop_signal_stage=src->stop_signal_stage;
}

static int parse_manifest(const char *path, struct nova_service *svc) {
    char buffer[FILE_BUFFER]; if (read_text_file(path, buffer, sizeof(buffer)) < 0) return -1;
    nova_zero(svc, sizeof(*svc)); svc->enabled=1; svc->autostart=1;
    unsigned int metadata_seen=0U;
    int capabilities_seen=0;
    int sandbox_seen=0;
    int directory_scope_seen=0;
    char *p=buffer;
    while (*p) {
        char *line=p; while (*p && *p!='\n') p++; if (*p=='\n') *p++='\0'; line=trim(line);
        if (!*line || *line=='#') continue;
        char *eq=line; while (*eq && *eq!='=') eq++; if (*eq!='=') continue; *eq++='\0';
        char *key=trim(line), *value=trim(eq);
        if (nova_streq(key,"name")) {
            if (!*value || nova_strlen(value)>=sizeof(svc->name)) return -1;
            copy_string(svc->name,sizeof(svc->name),value);
        }
        else if (nova_streq(key,"exec")) {
            if (!*value || nova_strlen(value)>=sizeof(svc->exec)) return -1;
            copy_string(svc->exec,sizeof(svc->exec),value);
        }
        else if (nova_streq(key,"restart")) svc->restart_always=nova_streq(value,"always");
        else if (nova_streq(key,"critical")) svc->critical=nova_streq(value,"yes");
        else if (nova_streq(key,"enabled")) svc->enabled=!nova_streq(value,"no");
        else if (nova_streq(key,"autostart")) {
            if (!nova_streq(value,"yes") && !nova_streq(value,"no")) return -1;
            svc->autostart=nova_streq(value,"yes");
        }
        else if (nova_streq(key,"kind")) {
            if (nova_streq(value,"application")) svc->is_application=1;
            else if (!nova_streq(value,"service")) return -1;
        }
        else if (nova_streq(key,"app_id")) {
            if (!safe_metadata_token(value,sizeof(svc->app_id))) return -1;
            copy_string(svc->app_id,sizeof(svc->app_id),value);
        }
        else if (nova_streq(key,"display_name")) {
            if (!safe_display_name(value,sizeof(svc->display_name))) return -1;
            copy_string(svc->display_name,sizeof(svc->display_name),value);
        }
        else if (nova_streq(key,"category")) {
            if ((metadata_seen&1U) || !application_category_from_name(value,&svc->category)) return -1;
            metadata_seen|=1U;
        }
        else if (nova_streq(key,"description")) {
            if ((metadata_seen&2U) || !safe_app_description(value,sizeof(svc->description))) return -1;
            copy_string(svc->description,sizeof(svc->description),value);
            metadata_seen|=2U;
        }
        else if (nova_streq(key,"icon_asset_id")) {
            if ((metadata_seen&4U) || !safe_metadata_token(value,sizeof(svc->icon_asset_id))) return -1;
            copy_string(svc->icon_asset_id,sizeof(svc->icon_asset_id),value);
            metadata_seen|=4U;
        }
        else if (nova_streq(key,"capabilities")) {
            if (capabilities_seen || !parse_application_capabilities(value,&svc->declared_capabilities)) return -1;
            capabilities_seen=1;
        }
        else if (nova_streq(key,"sandbox")) {
            if (sandbox_seen || !application_sandbox_profile_from_name(value,&svc->sandbox_profile,&svc->sandbox_required_flags)) return -1;
            sandbox_seen=1;
        }
        else if (nova_streq(key,"directory_scope")) {
            if(directory_scope_seen)return -1;directory_scope_seen=1;
            if(nova_streq(value,"user-home-read-only"))svc->files_user_home=1;
            else if(!nova_streq(value,"private-read-only"))return -1;
        }
        else if (nova_streq(key,"client_id")) {
            unsigned long v=0;
            if (!parse_unsigned_canonical(value,&v) || v==0 || v>0xffffffffUL) return -1;
            svc->client_id=(unsigned int)v;
        }
    }
    if(directory_scope_seen&&(!svc->is_application||!nova_streq(svc->app_id,"nova.files")||svc->client_id!=3||svc->sandbox_profile!=NOVA_APP_SANDBOX_PROFILE_FILES_READ_V1))return -1;
    if (!svc->name[0] || !svc->exec[0]) return -1;
    if (svc->is_application && (!svc->app_id[0] || !svc->display_name[0] || svc->client_id==0U || !(metadata_seen&1U) || !capabilities_seen || !sandbox_seen)) return -1;
    if (!svc->is_application && (svc->app_id[0] || svc->display_name[0] || svc->client_id!=0U || metadata_seen!=0U || capabilities_seen || sandbox_seen)) return -1;
    if (svc->is_application) {
        svc->effective_capabilities=application_policy_effective_capabilities(svc->declared_capabilities);
        if(!application_capability_policy_valid(svc) || !application_sandbox_policy_valid(svc)) return -1;
        svc->app_state=NOVA_APP_STATE_STOPPED;
    }
    return 0;
}

static int service_registry_conflicts(const struct nova_service *svc) {
    if (!svc) return 1;
    for (int i=0;i<g_service_count;i++) {
        const struct nova_service *cur=&g_services[i];
        if (nova_streq(cur->name,svc->name)) return 1;
        if (svc->is_application && cur->is_application) {
            if (nova_streq(cur->app_id,svc->app_id)) return 1;
            if (cur->client_id==svc->client_id) return 1;
        }
    }
    return 0;
}

static struct nova_service *find_application_by_id(const char *app_id) {
    if (!app_id || !safe_metadata_token(app_id,NOVA_APP_ID_BYTES)) return (struct nova_service *)0;
    for (int i=0;i<g_service_count;i++) {
        if (g_services[i].is_application && nova_streq(g_services[i].app_id,app_id)) return &g_services[i];
    }
    return (struct nova_service *)0;
}

static struct nova_service *find_application_by_client_id(unsigned int client_id) {
    if (client_id==0U) return (struct nova_service *)0;
    for (int i=0;i<g_service_count;i++) {
        if (g_services[i].is_application && g_services[i].client_id==client_id) return &g_services[i];
    }
    return (struct nova_service *)0;
}

static struct nova_service *find_application_by_pid(unsigned int process_id) {
    if(process_id==0U) return (struct nova_service *)0;
    for(int i=0;i<g_service_count;i++){
        struct nova_service *svc=&g_services[i];
        if(svc->is_application && svc->pid>0 && (unsigned long)svc->pid==(unsigned long)process_id) return svc;
    }
    return (struct nova_service *)0;
}

static unsigned int application_catalog_count(void) {
    unsigned int count=0U;
    for(int i=0;i<g_service_count;i++) if(g_services[i].is_application) count++;
    return count;
}

static struct nova_service *application_catalog_entry(unsigned int index) {
    unsigned int slot=0U;
    for(int i=0;i<g_service_count;i++){
        if(!g_services[i].is_application) continue;
        if(slot==index) return &g_services[i];
        slot++;
    }
    return (struct nova_service *)0;
}

static unsigned int application_catalog_flags(const struct nova_service *svc) {
    if(!svc || !svc->is_application) return 0U;
    unsigned int flags=NOVA_APP_CATALOG_FLAG_VISIBLE;
    if(svc->enabled) flags|=NOVA_APP_CATALOG_FLAG_LAUNCHABLE;
    return flags;
}

static unsigned int application_metadata_flags(const struct nova_service *svc) {
    if(!svc || !svc->is_application) return 0U;
    unsigned int flags=0U;
    if(svc->description[0]) flags|=NOVA_APP_METADATA_FLAG_HAS_DESCRIPTION;
    if(svc->icon_asset_id[0]) flags|=NOVA_APP_METADATA_FLAG_HAS_ICON;
    return flags;
}

static void write_app_catalog_state(void) {
    if(nova_getpid()!=1) return;
    long fd=nova_open_write(NOVA_APP_CATALOG_STATE_FILE,0644);
    if(fd<0) return;
    unsigned int count=application_catalog_count();
    nova_fd_putln((int)fd,"schema=2");
    nova_fd_puts((int)fd,"protocol="); nova_fd_put_u64((int)fd,NOVA_APP_CATALOG_PROTOCOL_VERSION); nova_fd_putln((int)fd,"");
    nova_fd_putln((int)fd,"metadata_extension=phase15-v1");
    nova_fd_puts((int)fd,"catalog_generation="); nova_fd_put_u64((int)fd,(unsigned long)g_app_catalog_generation); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"registry_count="); nova_fd_put_u64((int)fd,count); nova_fd_putln((int)fd,"");
    for(unsigned int index=0U;index<count;index++){
        struct nova_service *svc=application_catalog_entry(index);
        if(!svc) continue;
        unsigned int slot=index+1U;
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_id="); nova_fd_putln((int)fd,svc->app_id);
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_client_id="); nova_fd_put_u64((int)fd,svc->client_id); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_display_name="); nova_fd_putln((int)fd,svc->display_name);
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_state="); nova_fd_put_u64((int)fd,svc->app_state); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_flags="); nova_fd_put_u64((int)fd,application_catalog_flags(svc)); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_category="); nova_fd_put_u64((int)fd,svc->category); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_metadata_flags="); nova_fd_put_u64((int)fd,application_metadata_flags(svc)); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_description="); nova_fd_putln((int)fd,svc->description[0]?svc->description:"none");
        nova_fd_puts((int)fd,"entry"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_icon_asset_id="); nova_fd_putln((int)fd,svc->icon_asset_id[0]?svc->icon_asset_id:"none");
    }
    (void)nova_fsync((int)fd);
    (void)nova_close((int)fd);
}

static void write_capability_list(int fd,unsigned long long mask) {
    if(mask==0ULL){ nova_fd_puts(fd,"none"); return; }
    int first=1;
#define NOVA_CAP_STATE_TOKEN(bit,token) do { \
    if(mask&(bit)){ if(!first) nova_fd_puts(fd,","); nova_fd_puts(fd,(token)); first=0; } \
} while(0)
    NOVA_CAP_STATE_TOKEN(NOVA_APP_CAP_UI_SURFACE,NOVA_APP_CAP_TOKEN_UI_SURFACE);
    NOVA_CAP_STATE_TOKEN(NOVA_APP_CAP_INPUT_TEXT,NOVA_APP_CAP_TOKEN_INPUT_TEXT);
    NOVA_CAP_STATE_TOKEN(NOVA_APP_CAP_INPUT_TAB_CAPTURE,NOVA_APP_CAP_TOKEN_INPUT_TAB_CAPTURE);
    NOVA_CAP_STATE_TOKEN(NOVA_APP_CAP_CLIPBOARD_READ,NOVA_APP_CAP_TOKEN_CLIPBOARD_READ);
    NOVA_CAP_STATE_TOKEN(NOVA_APP_CAP_CLIPBOARD_WRITE,NOVA_APP_CAP_TOKEN_CLIPBOARD_WRITE);
    NOVA_CAP_STATE_TOKEN(NOVA_APP_CAP_RESOURCE_DEMO_READ,NOVA_APP_CAP_TOKEN_RESOURCE_DEMO_READ);
    NOVA_CAP_STATE_TOKEN(NOVA_APP_CAP_RESOURCE_NOTES_READ,NOVA_APP_CAP_TOKEN_RESOURCE_NOTES_READ);
#undef NOVA_CAP_STATE_TOKEN
}

static void write_app_security_state(void) {
    if(nova_getpid()!=1) return;
    long fd=nova_open_write(NOVA_APP_SECURITY_STATE_FILE,0644);
    if(fd<0) return;
    unsigned int count=application_catalog_count();
    nova_fd_putln((int)fd,"schema=1");
    nova_fd_putln((int)fd,"policy_kind=manifest-capability-policy-v1");
    nova_fd_puts((int)fd,"policy_authorized="); nova_fd_putln((int)fd,normal_boot_policy_satisfied()?"yes":"no");
    nova_fd_puts((int)fd,"security_protocol="); nova_fd_put_u64((int)fd,NOVA_APP_SECURITY_PROTOCOL_VERSION); nova_fd_putln((int)fd,"");
    /* Frozen Phase-16/17 state field describes the historical five capabilities.
     * Publish additive Phase-18 vocabulary under a separate key. */
    nova_fd_puts((int)fd,"known_capability_mask="); nova_fd_put_u64((int)fd,(unsigned long)NOVA_APP_CAP_PHASE16_KNOWN_MASK); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"phase18_known_capability_mask="); nova_fd_put_u64((int)fd,(unsigned long)NOVA_APP_CAP_PHASE18_KNOWN_MASK); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"phase18_step5_known_capability_mask="); nova_fd_put_u64((int)fd,(unsigned long)NOVA_APP_CAP_PHASE18_STEP5_KNOWN_MASK); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"policy_generation="); nova_fd_put_u64((int)fd,(unsigned long)g_app_policy_generation); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"registry_count="); nova_fd_put_u64((int)fd,count); nova_fd_putln((int)fd,"");
    for(unsigned int index=0U;index<count;index++){
        struct nova_service *svc=application_catalog_entry(index);
        if(!svc) continue;
        unsigned int slot=index+1U;
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_id="); nova_fd_putln((int)fd,svc->app_id);
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_client_id="); nova_fd_put_u64((int)fd,svc->client_id); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_declared_mask="); nova_fd_put_u64((int)fd,(unsigned long)svc->declared_capabilities); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_effective_mask="); nova_fd_put_u64((int)fd,(unsigned long)svc->effective_capabilities); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_declared="); write_capability_list((int)fd,svc->declared_capabilities); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_effective="); write_capability_list((int)fd,svc->effective_capabilities); nova_fd_putln((int)fd,"");
    }
    (void)nova_fsync((int)fd);
    (void)nova_close((int)fd);
}

static void write_app_lifecycle_state(void) {
    if(nova_getpid()!=1) return;
    long fd=nova_open_write(NOVA_APP_STATE_FILE,0644);
    if(fd<0) return;
    nova_fd_putln((int)fd,"schema=1");
    unsigned int app_count=0U;
    for(int i=0;i<g_service_count;i++) if(g_services[i].is_application) app_count++;
    nova_fd_puts((int)fd,"registry_count="); nova_fd_put_u64((int)fd,app_count); nova_fd_putln((int)fd,"");
    unsigned int slot=0U;
    for(int i=0;i<g_service_count;i++){
        struct nova_service *svc=&g_services[i];
        if(!svc->is_application) continue;
        slot++;
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_id="); nova_fd_putln((int)fd,svc->app_id);
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_client_id="); nova_fd_put_u64((int)fd,svc->client_id); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_state="); nova_fd_put_u64((int)fd,svc->app_state); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_pid="); nova_fd_put_u64((int)fd,svc->pid>0?(unsigned long)svc->pid:0UL); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_generation="); nova_fd_put_u64((int)fd,svc->app_generation); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_desired_running="); nova_fd_putln((int)fd,svc->desired_running?"yes":"no");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_restart_count="); nova_fd_put_u64((int)fd,svc->restart_count); nova_fd_putln((int)fd,"");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_autostart="); nova_fd_putln((int)fd,svc->autostart?"yes":"no");
        nova_fd_puts((int)fd,"app"); nova_fd_put_u64((int)fd,slot); nova_fd_puts((int)fd,"_stop_signal_stage="); nova_fd_put_u64((int)fd,svc->stop_signal_stage); nova_fd_putln((int)fd,"");
    }
    (void)nova_fsync((int)fd);
    (void)nova_close((int)fd);
    write_app_catalog_state();
    write_app_security_state();
}

static int application_registry_consistent(void) {
    if(application_catalog_count()>NOVA_APP_CATALOG_MAX_ENTRIES) return 0;
    for (int i=0;i<g_service_count;i++) {
        struct nova_service *svc=&g_services[i];
        if (!svc->is_application) continue;
        if (!safe_display_name(svc->display_name,sizeof(svc->display_name))) return 0;
        if (svc->category>NOVA_APP_CATEGORY_UTILITY) return 0;
        if (svc->description[0] && !safe_app_description(svc->description,sizeof(svc->description))) return 0;
        if (svc->icon_asset_id[0] && !safe_metadata_token(svc->icon_asset_id,sizeof(svc->icon_asset_id))) return 0;
        if (!application_capability_policy_valid(svc)) return 0;
        if (find_application_by_id(svc->app_id)!=svc) return 0;
        if (find_application_by_client_id(svc->client_id)!=svc) return 0;
    }
    return 1;
}

static void load_services(void) {
    g_boot_stage = "SERVICE_LOAD";
    char list[FILE_BUFFER]; long n=read_text_file("/System/Services/services.list",list,sizeof(list));
    if (n<0) { if (nova_getpid()==1) { g_boot_degraded=1; log_simple("ERROR","services","index_missing","service index not found"); } return; }
    char *p=list;
    int registry_overflow=0;
    while (*p) {
        char *line=p; while (*p && *p!='\n') p++; if (*p=='\n') *p++='\0'; line=trim(line);
        if (!*line || *line=='#') continue;
        if (g_service_count>=MAX_SERVICES) {
            registry_overflow=1;
            continue;
        }
        struct nova_service svc;
        if (parse_manifest(line,&svc)==0 && !service_registry_conflicts(&svc)) {
            copy_service(&g_services[g_service_count],&svc); g_service_count++;
        } else {
            g_boot_degraded=1; log_simple("WARN","services","manifest_invalid","invalid or conflicting service manifest skipped");
        }
    }
    if (registry_overflow) {
        g_service_count=0;
        g_boot_degraded=1;
        log_simple("ERROR","services","registry_capacity_exceeded","service index exceeds bounded registry capacity; all services blocked");
        return;
    }
    if (!application_registry_consistent()) {
        g_boot_degraded=1;
        log_simple("ERROR","services","application_registry_invalid","application registry consistency check failed");
    } else {
        g_app_catalog_generation=1ULL;
        g_app_policy_generation=1ULL;
        write_app_security_state();
    }
}

static const char *trust_name(void) {
    if (g_trust==TRUST_VERIFIED) return "VERIFIED";
    if (g_trust==TRUST_FAILED) return "FAILED";
    if (g_trust==TRUST_UNAVAILABLE) return "UNAVAILABLE";
    return "UNKNOWN";
}

static void verify_manifest_trust(void) {
    g_boot_stage = "TRUST";
    if (!nova_sha256_selftest()) {
        g_trust=TRUST_FAILED; g_boot_degraded=1;
        log_simple("ERROR","trust","selftest_failed","SHA-256 self-test failed before trust verification"); return;
    }
    char cert_hash[65];
    if (nova_sha256_file(ROOT_CERT_PATH, cert_hash) < 0) {
        g_trust=TRUST_UNAVAILABLE; g_boot_degraded=1;
        log_simple("ERROR","trust","root_missing","System copy of pinned root certificate unavailable"); return;
    }
    if (!nova_streq(cert_hash, NOVA_ROOT_CERT_SHA256)) {
        g_trust=TRUST_FAILED; g_boot_degraded=1;
        log_simple("ERROR","trust","root_pin_mismatch","System root certificate does not match the pin embedded in /init"); return;
    }
    if(!ensure_pinned_root_key(PREBOOT_ROOT_CERT_PATH)){
        g_trust=TRUST_FAILED; g_boot_degraded=1;
        log_simple("ERROR","trust","preboot_key_failed","preboot pinned root could not be loaded into the kernel keyring"); return;
    }
    if(!verify_signature_with_loaded_root(INTEGRITY_MANIFEST,INTEGRITY_SIGNATURE)){
        g_trust=TRUST_FAILED; g_boot_degraded=1;
        log_simple("ERROR","trust","signature_failed","integrity manifest signature verification failed"); return;
    }
    g_trust=TRUST_VERIFIED;
    log_simple("INFO","trust","signature_verified","signed integrity manifest verified by pinned NovaOS root");
}

static const char *integrity_name(void) {
    if (g_integrity==INTEGRITY_VERIFIED) return "VERIFIED";
    if (g_integrity==INTEGRITY_FAILED) return "FAILED";
    if (g_integrity==INTEGRITY_UNAVAILABLE) return "UNAVAILABLE";
    return "UNKNOWN";
}

static const char *verity_state_name(enum verity_state state) {
    if (state==VERITY_ENFORCED) return "ENFORCED";
    if (state==VERITY_FAILED) return "FAILED";
    if (state==VERITY_UNAVAILABLE) return "UNAVAILABLE";
    return "UNKNOWN";
}
static const char *verity_name(void) { return verity_state_name(g_verity); }
static const char *recovery_verity_name(void) { return verity_state_name(g_recovery_verity); }

static int hex64(const char *s) {
    for (int i=0;i<64;i++) { char c=s[i]; if (!c) return 0; if (!((c>='0'&&c<='9')||(c>='a'&&c<='f'))) return 0; }
    return s[64]==' ' || s[64]=='\t';
}

static void verify_integrity_manifest(void) {
    g_boot_stage = "INTEGRITY";
    if (g_trust != TRUST_VERIFIED) {
        g_integrity=INTEGRITY_UNAVAILABLE; g_boot_degraded=1;
        log_simple("ERROR","integrity","trust_required","integrity manifest rejected because its signature is not trusted"); return;
    }
    if (!nova_sha256_selftest()) {
        g_integrity=INTEGRITY_FAILED; g_boot_degraded=1;
        log_simple("ERROR","integrity","selftest_failed","SHA-256 self-test failed"); return;
    }
    char buffer[INTEGRITY_BUFFER]; long n=read_text_file(INTEGRITY_MANIFEST,buffer,sizeof(buffer));
    if (n<0) {
        g_integrity=INTEGRITY_UNAVAILABLE; g_boot_degraded=1;
        log_simple("ERROR","integrity","manifest_missing","integrity manifest unavailable"); return;
    }
    g_integrity=INTEGRITY_VERIFIED;
    char *p=buffer;
    while (*p) {
        char *line=p; while (*p && *p!='\n') p++; if (*p=='\n') *p++='\0'; line=trim(line);
        if (!*line || *line=='#') continue;
        g_integrity_total++;
        if (!hex64(line)) { g_integrity_failed++; g_integrity=INTEGRITY_FAILED; continue; }
        char expected[65]; for(int i=0;i<64;i++) expected[i]=line[i]; expected[64]='\0';
        char *path=line+64; while (*path==' '||*path=='\t') path++;
        char actual[65];
        if (!*path || nova_sha256_file(path,actual)<0 || !nova_streq(expected,actual)) {
            g_integrity_failed++; g_integrity=INTEGRITY_FAILED; log_integrity_failure(*path?path:"<invalid-path>");
        } else g_integrity_verified++;
    }
    if (g_integrity_total==0) g_integrity=INTEGRITY_FAILED;
    if (g_integrity!=INTEGRITY_VERIFIED) { g_boot_degraded=1; log_simple("ERROR","integrity","verification_failed","system integrity verification failed"); }
    else log_simple("INFO","integrity","verified","system integrity manifest verified");
}

static struct nova_service *find_service(const char *name) {
    for(int i=0;i<g_service_count;i++) if(nova_streq(g_services[i].name,name)) return &g_services[i];
    return (struct nova_service *)0;
}

static int service_execution_allowed(void) {
    return normal_boot_policy_satisfied();
}

static void start_service(struct nova_service *svc) {
    if (!svc || !svc->enabled || svc->pid>0) return;
    if (!service_execution_allowed()) { log_service("WARN","start_blocked",svc,-1); return; }
    if (svc->is_application && !sandbox_prepare_persistent_data(svc)) {
        svc->app_state=NOVA_APP_STATE_FAILED; svc->desired_running=0; write_app_lifecycle_state();
        log_service("ERROR","sandbox_data_failed",svc,-1);
        return;
    }
    if (svc->is_application) svc->app_state=NOVA_APP_STATE_STARTING;
    long pid=nova_clone_process();
    if(pid<0){
        if(svc->is_application){ svc->app_state=NOVA_APP_STATE_FAILED; svc->desired_running=0; write_app_lifecycle_state(); }
        if(svc->critical) g_boot_degraded=1;
        log_service("ERROR","clone_failed",svc,-1);
        return;
    }
    if(pid==0){
        if(svc->is_application && !apply_application_sandbox(svc)){
            nova_fd_putln(NOVA_STDERR,"novad: application sandbox setup failed");
            nova_exit(126);
        }
        char *argv[2]={svc->exec,(char*)0}; char *envp[1]={(char*)0};
        nova_execve(svc->exec,argv,envp); nova_exit(127);
    }
    svc->pid=pid; svc->desired_running=1;
    if(svc->is_application){
        svc->app_generation++; svc->app_state=NOVA_APP_STATE_RUNNING;
        svc->stop_deadline_ns=0ULL; svc->stop_fallback_ticks=0U; svc->stop_signal_stage=0U;
    }
    log_service("INFO","started",svc,-1);
    if(svc->is_application) write_app_lifecycle_state();
}

static void handle_service_exit(struct nova_service *svc,int status) {
    if(!svc) return;
    log_service("WARN","exited",svc,status);
    svc->pid=0;
    if(svc->is_application){ svc->stop_deadline_ns=0ULL; svc->stop_fallback_ticks=0U; svc->stop_signal_stage=0U; }
    if(svc->desired_running && svc->restart_always){
        svc->restart_count++;
        if(svc->is_application) svc->app_state=NOVA_APP_STATE_RESTARTING;
        log_service("INFO","restart_policy",svc,-1);
        start_service(svc);
    } else {
        if(svc->is_application) svc->app_state=NOVA_APP_STATE_STOPPED;
        if(svc->critical && svc->desired_running) g_boot_degraded=1;
    }
    if(svc->is_application) write_app_lifecycle_state();
}

static void reap_services(void) {
    for(;;){ int status=0; long pid=nova_wait4(-1,&status,NOVA_WNOHANG); if(pid<=0) break;
        for(int i=0;i<g_service_count;i++) if(g_services[i].pid==pid){ handle_service_exit(&g_services[i],status); break; }
    }
}

static void fill_app_addr(struct nova_sockaddr_un *addr) {
    nova_zero(addr,sizeof(*addr));
    addr->sun_family=NOVA_AF_UNIX;
    const char *src=NOVA_APP_SOCKET_PATH;
    unsigned long i=0;
    while(src[i] && i+1UL<sizeof(addr->sun_path)){ addr->sun_path[i]=src[i]; i++; }
    addr->sun_path[i]='\0';
}

static int setup_app_lifecycle_listener(void) {
    struct nova_sockaddr_un addr;
    fill_app_addr(&addr);
    (void)nova_unlink(NOVA_APP_SOCKET_PATH);
    long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,0);
    if(fd<0){ log_simple("ERROR","app_lifecycle","socket_failed","application lifecycle socket creation failed"); return -1; }
    long rc=nova_bind((int)fd,&addr,sizeof(addr));
    if(rc<0){ nova_close((int)fd); log_simple("ERROR","app_lifecycle","bind_failed","application lifecycle socket bind failed"); return -1; }
    rc=nova_listen((int)fd,1);
    if(rc<0){ nova_close((int)fd); (void)nova_unlink(NOVA_APP_SOCKET_PATH); log_simple("ERROR","app_lifecycle","listen_failed","application lifecycle socket listen failed"); return -1; }
    log_simple("INFO","app_lifecycle","listening","application lifecycle IPC is listening");
    return (int)fd;
}

static void fill_app_catalog_addr(struct nova_sockaddr_un *addr) {
    nova_zero(addr,sizeof(*addr));
    addr->sun_family=NOVA_AF_UNIX;
    const char *src=NOVA_APP_CATALOG_SOCKET_PATH;
    unsigned long i=0;
    while(src[i] && i+1UL<sizeof(addr->sun_path)){ addr->sun_path[i]=src[i]; i++; }
    addr->sun_path[i]='\0';
}

static int setup_app_catalog_listener(void) {
    struct nova_sockaddr_un addr;
    fill_app_catalog_addr(&addr);
    (void)nova_unlink(NOVA_APP_CATALOG_SOCKET_PATH);
    long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,0);
    if(fd<0){ log_simple("ERROR","app_catalog","socket_failed","application catalog socket creation failed"); return -1; }
    long rc=nova_bind((int)fd,&addr,sizeof(addr));
    if(rc<0){ nova_close((int)fd); log_simple("ERROR","app_catalog","bind_failed","application catalog socket bind failed"); return -1; }
    rc=nova_listen((int)fd,1);
    if(rc<0){ nova_close((int)fd); (void)nova_unlink(NOVA_APP_CATALOG_SOCKET_PATH); log_simple("ERROR","app_catalog","listen_failed","application catalog socket listen failed"); return -1; }
    log_simple("INFO","app_catalog","listening","read-only application catalog IPC is listening");
    return (int)fd;
}

static void fill_app_security_addr(struct nova_sockaddr_un *addr) {
    nova_zero(addr,sizeof(*addr));
    addr->sun_family=NOVA_AF_UNIX;
    const char *src=NOVA_APP_SECURITY_SOCKET_PATH;
    unsigned long i=0;
    while(src[i] && i+1UL<sizeof(addr->sun_path)){ addr->sun_path[i]=src[i]; i++; }
    addr->sun_path[i]='\0';
}

static int peer_pid_for_socket(int fd,unsigned int *pid_out) {
    if(fd<0 || !pid_out) return 0;
    struct nova_ucred cred;
    unsigned int len=(unsigned int)sizeof(cred);
    nova_zero(&cred,sizeof(cred));
    long rc=nova_getsockopt(fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED,&cred,&len);
    if(rc<0 || len!=(unsigned int)sizeof(cred) || cred.pid<=0) return 0;
    *pid_out=(unsigned int)cred.pid;
    return 1;
}

static int setup_app_security_listener(void) {
    struct nova_sockaddr_un addr;
    fill_app_security_addr(&addr);
    (void)nova_unlink(NOVA_APP_SECURITY_SOCKET_PATH);
    long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,0);
    if(fd<0){ log_simple("ERROR","app_security","socket_failed","application security socket creation failed"); return -1; }
    long rc=nova_bind((int)fd,&addr,sizeof(addr));
    if(rc<0){ nova_close((int)fd); log_simple("ERROR","app_security","bind_failed","application security socket bind failed"); return -1; }
    rc=nova_listen((int)fd,1);
    if(rc<0){ nova_close((int)fd); (void)nova_unlink(NOVA_APP_SECURITY_SOCKET_PATH); log_simple("ERROR","app_security","listen_failed","application security socket listen failed"); return -1; }
    log_simple("INFO","app_security","listening","authenticated application-principal IPC is listening");
    return (int)fd;
}

static int overview_peer_authorized(int fd){
 struct nova_ucred peer;unsigned int len=sizeof(peer);nova_zero(&peer,sizeof(peer));struct nova_service*c=find_service("compositor");
 return c&&c->pid>0&&nova_getsockopt(fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED,&peer,&len)==0&&len==sizeof(peer)&&peer.uid==0&&peer.pid>0&&c->pid==peer.pid;
}
static void ordered_shutdown(int reboot_command);
static unsigned long long monotonic_now_ns(void);
static int g_power_pending,g_power_client=-1;static unsigned long long g_power_deadline;
static int setup_power_listener(void){struct nova_sockaddr_un a;nova_zero(&a,sizeof a);a.sun_family=NOVA_AF_UNIX;copy_string(a.sun_path,sizeof a.sun_path,NOVA_POWER_PATH);(void)nova_unlink(NOVA_POWER_PATH);long f=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK|02000000,0);if(f<0)return -1;if(nova_bind((int)f,&a,sizeof a)<0||nova_listen((int)f,1)<0){nova_close((int)f);return -1;}return (int)f;}
static void poll_power(int listener){
 if(listener<0||g_power_pending)return;
 if(g_power_client<0){long fd=nova_accept4(listener,NOVA_SOCK_NONBLOCK|02000000);if(fd<0)return;if(!overview_peer_authorized((int)fd)){nova_close((int)fd);return;}g_power_client=(int)fd;g_power_deadline=monotonic_now_ns()+1000000000ULL;}
 struct nova_power_message q;nova_zero(&q,sizeof q);struct nova_iovec io={&q,sizeof q};struct nova_msghdr msg;nova_zero(&msg,sizeof msg);msg.msg_iov=&io;msg.msg_iovlen=1;long n=nova_recvmsg(g_power_client,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_CMSG_CLOEXEC);
 if(n==-NOVA_EAGAIN&&monotonic_now_ns()<g_power_deadline)return;
 if(n==sizeof q&&!(msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC))&&nova_power_valid(&q)&&overview_peer_authorized(g_power_client)){
  struct nova_msghdr reply;nova_zero(&reply,sizeof reply);reply.msg_iov=&io;reply.msg_iovlen=1;(void)nova_sendmsg(g_power_client,&reply,NOVA_MSG_DONTWAIT|NOVA_MSG_NOSIGNAL);g_power_pending=1;
 }
 nova_close(g_power_client);g_power_client=-1;
}
static void fill_overview_reply(struct nova_overview_reply*out,unsigned int sequence){
 nova_zero(out,sizeof(*out));out->magic=NOVA_OVERVIEW_MAGIC;out->version=1;out->size=sizeof(*out);out->sequence=sequence;
 for(int i=0;i<g_service_count;i++){struct nova_service*v=&g_services[i];out->services_total++;if(v->pid>0)out->services_running++;if(v->is_application){out->apps_total++;if(v->pid>0)out->apps_running++;}if(v->restart_count>1000000U-out->restarts)out->restarts=1000000U;else out->restarts+=(unsigned int)v->restart_count;}
 out->secure_boot=g_firmware_trust==FIRMWARE_TRUST_UNKNOWN?0U:g_firmware_trust==FIRMWARE_TRUST_DISABLED?2U:(g_firmware_trust==FIRMWARE_TRUST_VERIFIED&&g_firmware_secure_boot)?1U:3U;
 out->tpm=g_tpm_state==TPM_STATE_UNKNOWN?0U:g_tpm_state==TPM_STATE_READY?1U:g_tpm_state==TPM_STATE_FAILED?2U:3U;
 out->verity=g_verity==VERITY_UNKNOWN?0U:g_verity==VERITY_ENFORCED?1U:g_verity==VERITY_FAILED?2U:3U;out->degraded=g_boot_degraded?1:0;
 struct nova_timespec now;if(nova_clock_gettime(1,&now)==0&&now.tv_sec>=0&&now.tv_nsec>=0)out->monotonic_ns=(unsigned long long)now.tv_sec*1000000000ULL+(unsigned long long)now.tv_nsec;
}
static int process_overview_packet(int fd){
 if(!overview_peer_authorized(fd))return -1;
 struct nova_overview_request q;nova_zero(&q,sizeof(q));struct nova_iovec io={&q,sizeof(q)};struct nova_msghdr msg;nova_zero(&msg,sizeof(msg));msg.msg_iov=&io;msg.msg_iovlen=1;
 long n=nova_recvmsg(fd,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_CMSG_CLOEXEC);if(n==-NOVA_EAGAIN||n==-NOVA_EINTR)return 0;
 if(n!=sizeof(q)||(msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC))||!nova_overview_request_valid(&q))return -1;
 struct nova_overview_reply reply;fill_overview_reply(&reply,q.sequence);io.iov_base=&reply;io.iov_len=sizeof(reply);nova_zero(&msg,sizeof(msg));msg.msg_iov=&io;msg.msg_iovlen=1;
 return nova_sendmsg(fd,&msg,NOVA_MSG_DONTWAIT|NOVA_MSG_NOSIGNAL)==sizeof(reply)?0:-1;
}
static int setup_overview_listener(void){
 struct nova_sockaddr_un addr;nova_zero(&addr,sizeof(addr));addr.sun_family=NOVA_AF_UNIX;copy_string(addr.sun_path,sizeof(addr.sun_path),NOVA_OVERVIEW_PATH);
 (void)nova_unlink(NOVA_OVERVIEW_PATH);long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK|02000000,0);if(fd<0){log_simple("WARN","overview","socket_failed","overview unavailable");return -1;}
 if(nova_bind((int)fd,&addr,sizeof(addr))<0||nova_listen((int)fd,1)<0){nova_close((int)fd);log_simple("WARN","overview","listener_failed","overview unavailable");return -1;}log_simple("INFO","overview","listening","read-only supervised compositor telemetry");return (int)fd;
}
static void poll_overview(int listener,int*client){
 if(listener<0)return;struct nova_pollfd fds[2]={{listener,NOVA_POLLIN,0},{*client,NOVA_POLLIN|NOVA_POLLHUP|NOVA_POLLERR,0}};struct nova_timespec zero={0,0};if(nova_poll(fds,2,&zero)<0)return;
 if(*client>=0&&((fds[1].revents&(NOVA_POLLHUP|NOVA_POLLERR))||((fds[1].revents&NOVA_POLLIN)&&process_overview_packet(*client)<0))){nova_close(*client);*client=-1;}
 if(fds[0].revents&NOVA_POLLIN){long fd=nova_accept4(listener,NOVA_SOCK_NONBLOCK|02000000);if(fd>=0){if(*client>=0||!overview_peer_authorized((int)fd))nova_close((int)fd);else *client=(int)fd;}}
}

static int app_catalog_request_valid(const struct nova_app_catalog_request *request) {
    if(!request) return 0;
    if(request->magic!=NOVA_APP_CATALOG_PROTOCOL_MAGIC || request->version!=NOVA_APP_CATALOG_PROTOCOL_VERSION) return 0;
    if(request->size!=(unsigned int)sizeof(*request) || request->flags!=0U || request->reserved!=0U) return 0;
    if(request->opcode==NOVA_APP_CATALOG_OP_HELLO) return request->index==0U;
    if(request->opcode==NOVA_APP_CATALOG_OP_GET_ENTRY || request->opcode==NOVA_APP_CATALOG_OP_GET_METADATA) return 1;
    return 0;
}

static void fill_app_catalog_reply(struct nova_app_catalog_reply *reply,
                                   const struct nova_app_catalog_request *request,
                                   unsigned int opcode,unsigned int result,
                                   const struct nova_service *svc) {
    nova_zero(reply,sizeof(*reply));
    reply->magic=NOVA_APP_CATALOG_PROTOCOL_MAGIC;
    reply->version=NOVA_APP_CATALOG_PROTOCOL_VERSION;
    reply->opcode=opcode;
    reply->size=sizeof(*reply);
    if(request){ reply->request_id=request->request_id; reply->index=request->index; }
    reply->total_count=application_catalog_count();
    reply->result=result;
    reply->catalog_generation=g_app_catalog_generation;
    if(svc){
        reply->client_id=svc->client_id;
        reply->state=svc->app_state;
        reply->catalog_flags=application_catalog_flags(svc);
        copy_string(reply->app_id,sizeof(reply->app_id),svc->app_id);
        copy_string(reply->display_name,sizeof(reply->display_name),svc->display_name);
    }
}

static int send_app_catalog_reply(int fd,const struct nova_app_catalog_reply *reply) {
    long n=nova_write(fd,reply,sizeof(*reply));
    return n==(long)sizeof(*reply) ? 0 : -1;
}

static void fill_app_metadata_reply(struct nova_app_metadata_reply *reply,
                                    const struct nova_app_catalog_request *request,
                                    const struct nova_service *svc) {
    nova_zero(reply,sizeof(*reply));
    reply->magic=NOVA_APP_CATALOG_PROTOCOL_MAGIC;
    reply->version=NOVA_APP_CATALOG_PROTOCOL_VERSION;
    reply->opcode=NOVA_APP_CATALOG_OP_METADATA;
    reply->size=sizeof(*reply);
    reply->request_id=request?request->request_id:0U;
    reply->index=request?request->index:0U;
    reply->total_count=application_catalog_count();
    reply->result=NOVA_APP_CATALOG_RESULT_OK;
    reply->catalog_generation=g_app_catalog_generation;
    if(!svc) return;
    reply->client_id=svc->client_id;
    reply->category=svc->category;
    reply->metadata_flags=application_metadata_flags(svc);
    copy_string(reply->app_id,sizeof(reply->app_id),svc->app_id);
    copy_string(reply->description,sizeof(reply->description),svc->description);
    copy_string(reply->icon_asset_id,sizeof(reply->icon_asset_id),svc->icon_asset_id);
}

static int send_app_metadata_reply(int fd,const struct nova_app_metadata_reply *reply) {
    long n=nova_write(fd,reply,sizeof(*reply));
    return n==(long)sizeof(*reply) ? 0 : -1;
}

static int process_app_catalog_packet(int fd) {
    struct nova_app_catalog_request request;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    nova_zero(&request,sizeof(request));
    nova_zero(&iov,sizeof(iov));
    nova_zero(&msg,sizeof(msg));
    iov.iov_base=&request;
    iov.iov_len=sizeof(request);
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;
    long n=nova_recvmsg(fd,&msg,0);
    if(n==-NOVA_EAGAIN || n==-NOVA_EINTR) return 0;
    if(n<=0) return -1;

    struct nova_app_catalog_reply reply;
    if(n!=(long)sizeof(request) || (msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)) || !app_catalog_request_valid(&request)){
        fill_app_catalog_reply(&reply,&request,NOVA_APP_CATALOG_OP_ERROR,NOVA_APP_CATALOG_RESULT_BAD_REQUEST,(const struct nova_service *)0);
        return send_app_catalog_reply(fd,&reply)<0 ? -1 : 0;
    }
    if(!normal_boot_policy_satisfied()){
        fill_app_catalog_reply(&reply,&request,NOVA_APP_CATALOG_OP_ERROR,NOVA_APP_CATALOG_RESULT_DENIED,(const struct nova_service *)0);
        return send_app_catalog_reply(fd,&reply)<0 ? -1 : 0;
    }
    if(request.opcode==NOVA_APP_CATALOG_OP_HELLO){
        fill_app_catalog_reply(&reply,&request,NOVA_APP_CATALOG_OP_HELLO_REPLY,NOVA_APP_CATALOG_RESULT_OK,(const struct nova_service *)0);
        return send_app_catalog_reply(fd,&reply)<0 ? -1 : 0;
    }
    struct nova_service *svc=application_catalog_entry(request.index);
    if(!svc){
        fill_app_catalog_reply(&reply,&request,NOVA_APP_CATALOG_OP_END,NOVA_APP_CATALOG_RESULT_OK,(const struct nova_service *)0);
        return send_app_catalog_reply(fd,&reply)<0 ? -1 : 0;
    }
    if(request.opcode==NOVA_APP_CATALOG_OP_GET_METADATA){
        struct nova_app_metadata_reply metadata;
        fill_app_metadata_reply(&metadata,&request,svc);
        return send_app_metadata_reply(fd,&metadata)<0 ? -1 : 0;
    }
    fill_app_catalog_reply(&reply,&request,NOVA_APP_CATALOG_OP_ENTRY,NOVA_APP_CATALOG_RESULT_OK,svc);
    return send_app_catalog_reply(fd,&reply)<0 ? -1 : 0;
}

static void disconnect_app_catalog_client(int *client_fd) {
    if(!client_fd || *client_fd<0) return;
    nova_close(*client_fd);
    *client_fd=-1;
    log_simple("INFO","app_catalog","client_disconnected","application catalog IPC client disconnected");
}

static void accept_app_catalog_client(int listener,int *client_fd) {
    if(listener<0 || !client_fd) return;
    long fd=nova_accept4(listener,NOVA_SOCK_NONBLOCK);
    if(fd<0) return;
    if(*client_fd>=0){ nova_close((int)fd); return; }
    *client_fd=(int)fd;
    log_simple("INFO","app_catalog","client_connected","application catalog IPC client connected");
}

static int app_id_frame_valid(const char app_id[NOVA_APP_ID_BYTES],int allow_empty) {
    unsigned long nul=NOVA_APP_ID_BYTES;
    for(unsigned long i=0;i<NOVA_APP_ID_BYTES;i++) if(app_id[i]=='\0'){ nul=i; break; }
    if(nul==NOVA_APP_ID_BYTES) return 0;
    if(nul==0UL) return allow_empty;
    return safe_metadata_token(app_id,NOVA_APP_ID_BYTES);
}

static void fill_app_status(struct nova_app_message *reply,const struct nova_app_message *request,
                            const struct nova_service *svc,unsigned int result) {
    nova_zero(reply,sizeof(*reply));
    reply->magic=NOVA_APP_PROTOCOL_MAGIC;
    reply->version=NOVA_APP_PROTOCOL_VERSION;
    reply->opcode=NOVA_APP_OP_STATUS;
    reply->size=(unsigned int)sizeof(*reply);
    reply->request_id=request?request->request_id:0U;
    reply->result=result;
    if(svc){
        reply->generation=svc->app_generation;
        reply->pid=(long long)svc->pid;
        reply->state=svc->app_state;
        copy_string(reply->app_id,sizeof(reply->app_id),svc->app_id);
    } else if(request && app_id_frame_valid(request->app_id,1)) {
        copy_string(reply->app_id,sizeof(reply->app_id),request->app_id);
    }
}

static unsigned int application_launch_request(struct nova_service *svc) {
    if(!svc) return NOVA_APP_RESULT_UNKNOWN_APP;
    if(!service_execution_allowed()) return NOVA_APP_RESULT_DENIED;
    if(svc->pid>0 || svc->app_state==NOVA_APP_STATE_STARTING || svc->app_state==NOVA_APP_STATE_RUNNING ||
       svc->app_state==NOVA_APP_STATE_STOPPING || svc->app_state==NOVA_APP_STATE_RESTARTING)
        return NOVA_APP_RESULT_ALREADY_RUNNING;
    svc->desired_running=1;
    start_service(svc);
    if(svc->pid<=0 || svc->app_state!=NOVA_APP_STATE_RUNNING) return NOVA_APP_RESULT_SPAWN_FAILED;
    return NOVA_APP_RESULT_OK;
}

static unsigned long long monotonic_now_ns(void) {
    struct nova_timespec ts;
    nova_zero(&ts,sizeof(ts));
    if(nova_clock_gettime(NOVA_CLOCK_MONOTONIC,&ts)<0) return 0ULL;
    if(ts.tv_sec<0 || ts.tv_nsec<0 || ts.tv_nsec>=1000000000L) return 0ULL;
    return (unsigned long long)ts.tv_sec*1000000000ULL+(unsigned long long)ts.tv_nsec;
}

static unsigned long long monotonic_deadline(unsigned long long now,unsigned long long delta) {
    if(!now) return 0ULL;
    if((~0ULL)-now<delta) return ~0ULL;
    return now+delta;
}

static unsigned int application_stop_request(struct nova_service *svc) {
    if(!svc) return NOVA_APP_RESULT_UNKNOWN_APP;
    if(svc->pid<=0){
        svc->desired_running=0; svc->app_state=NOVA_APP_STATE_STOPPED;
        svc->stop_deadline_ns=0ULL; svc->stop_fallback_ticks=0U; svc->stop_signal_stage=0U;
        write_app_lifecycle_state();
        return NOVA_APP_RESULT_NOT_RUNNING;
    }
    svc->desired_running=0;
    if(svc->app_state==NOVA_APP_STATE_STOPPING) return NOVA_APP_RESULT_OK;
    svc->app_state=NOVA_APP_STATE_STOPPING;
    svc->stop_signal_stage=0U;
    svc->stop_fallback_ticks=NOVA_APP_STOP_GRACE_FALLBACK_TICKS;
    svc->stop_deadline_ns=monotonic_deadline(monotonic_now_ns(),NOVA_APP_STOP_GRACE_NS);
    log_service("INFO","app_stop_graceful",svc,-1);
    write_app_lifecycle_state();
    return NOVA_APP_RESULT_OK;
}

static void enforce_application_stop_deadlines(void) {
    unsigned long long now=monotonic_now_ns();
    for(int i=0;i<g_service_count;i++){
        struct nova_service *svc=&g_services[i];
        if(!svc->is_application || svc->pid<=0 || svc->app_state!=NOVA_APP_STATE_STOPPING) continue;
        int expired=0;
        if(now && svc->stop_deadline_ns) expired=now>=svc->stop_deadline_ns;
        else if(svc->stop_fallback_ticks>0U) svc->stop_fallback_ticks--;
        else expired=1;
        if(!expired) continue;
        if(svc->stop_signal_stage==0U){
            log_service("WARN","app_stop_sigterm",svc,-1);
            (void)nova_kill(svc->pid,NOVA_SIGTERM);
            svc->stop_signal_stage=1U;
            svc->stop_fallback_ticks=NOVA_APP_STOP_TERM_FALLBACK_TICKS;
            svc->stop_deadline_ns=monotonic_deadline(now,NOVA_APP_STOP_TERM_NS);
            write_app_lifecycle_state();
        } else if(svc->stop_signal_stage==1U){
            log_service("ERROR","app_stop_sigkill",svc,-1);
            (void)nova_kill(svc->pid,NOVA_SIGKILL);
            svc->stop_signal_stage=2U;
            svc->stop_fallback_ticks=0U;
            svc->stop_deadline_ns=0ULL;
            write_app_lifecycle_state();
        }
    }
}

static int app_request_header_valid(const struct nova_app_message *request) {
    if(!request) return 0;
    if(request->magic!=NOVA_APP_PROTOCOL_MAGIC || request->version!=NOVA_APP_PROTOCOL_VERSION) return 0;
    if(request->size!=(unsigned int)sizeof(*request) || request->flags!=0U) return 0;
    if(request->generation!=0ULL || request->pid!=0 || request->state!=0U || request->result!=0U) return 0;
    if(request->opcode==NOVA_APP_OP_HELLO) return app_id_frame_valid(request->app_id,1) && request->app_id[0]=='\0';
    if(request->opcode==NOVA_APP_OP_LAUNCH || request->opcode==NOVA_APP_OP_STOP || request->opcode==NOVA_APP_OP_QUERY)
        return app_id_frame_valid(request->app_id,0);
    return 0;
}

static int send_app_reply(int fd,const struct nova_app_message *reply) {
    long n=nova_write(fd,reply,sizeof(*reply));
    return n==(long)sizeof(*reply) ? 0 : -1;
}

static int process_app_lifecycle_packet(int fd) {
    struct nova_app_message request;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    nova_zero(&request,sizeof(request));
    nova_zero(&iov,sizeof(iov));
    nova_zero(&msg,sizeof(msg));
    iov.iov_base=&request;
    iov.iov_len=sizeof(request);
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;
    long n=nova_recvmsg(fd,&msg,0);
    if(n==-NOVA_EAGAIN || n==-NOVA_EINTR) return 0;
    if(n<=0) return -1;

    struct nova_app_message reply;
    if(n!=(long)sizeof(request) || (msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)) || !app_request_header_valid(&request)){
        fill_app_status(&reply,&request,(struct nova_service *)0,NOVA_APP_RESULT_BAD_REQUEST);
        return send_app_reply(fd,&reply)<0 ? -1 : 0;
    }

    if(request.opcode==NOVA_APP_OP_HELLO){
        fill_app_status(&reply,&request,(struct nova_service *)0,NOVA_APP_RESULT_OK);
        reply.state=NOVA_APP_STATE_UNKNOWN;
        return send_app_reply(fd,&reply)<0 ? -1 : 0;
    }

    struct nova_service *svc=find_application_by_id(request.app_id);
    unsigned int result=NOVA_APP_RESULT_UNKNOWN_APP;
    if(svc){
        if(request.opcode==NOVA_APP_OP_LAUNCH) result=application_launch_request(svc);
        else if(request.opcode==NOVA_APP_OP_STOP) result=application_stop_request(svc);
        else if(request.opcode==NOVA_APP_OP_QUERY) result=NOVA_APP_RESULT_OK;
    }
    fill_app_status(&reply,&request,svc,result);
    return send_app_reply(fd,&reply)<0 ? -1 : 0;
}

static void disconnect_app_lifecycle_client(int *client_fd) {
    if(!client_fd || *client_fd<0) return;
    nova_close(*client_fd);
    *client_fd=-1;
    log_simple("INFO","app_lifecycle","client_disconnected","application lifecycle IPC client disconnected");
}

static void accept_app_lifecycle_client(int listener,int *client_fd) {
    if(listener<0 || !client_fd) return;
    long fd=nova_accept4(listener,NOVA_SOCK_NONBLOCK);
    if(fd<0) return;
    if(*client_fd>=0){ nova_close((int)fd); return; }
    *client_fd=(int)fd;
    log_simple("INFO","app_lifecycle","client_connected","application lifecycle IPC client connected");
}

static int console_consume_char(char *line,unsigned long *line_len,unsigned long capacity) {
    char c=0;
    long n=nova_read(NOVA_STDIN,&c,1);
    if(n==0) return nova_getpid()==1 ? 1 : 0;
    if(n<0){ if(n==-NOVA_EINTR || n==-NOVA_EAGAIN) return 1; nova_putln("console read failed"); return 1; }
    if(c=='\n' || c=='\r'){
        line[*line_len]='\0';
        if(*line_len>0 && !execute_command(line)) return 0;
        *line_len=0;
        nova_puts("nova> ");
        return 1;
    }
    if((c=='\b' || c==127) && *line_len>0){ (*line_len)--; return 1; }
    if(c>=32 && c<127 && *line_len+1UL<capacity) line[(*line_len)++]=c;
    return 1;
}


static void fill_resource_security_addr(struct nova_sockaddr_un *addr) {
    nova_zero(addr,sizeof(*addr));
    addr->sun_family=NOVA_AF_UNIX;
    const char *src=NOVA_RESOURCE_BROKER_AUTHORITY_SOCKET_PATH;
    unsigned long i=0U;
    while(src[i] && i+1UL<sizeof(addr->sun_path)){ addr->sun_path[i]=src[i]; i++; }
    addr->sun_path[i]='\0';
}

static int setup_resource_security_listener(void) {
    struct nova_sockaddr_un addr;
    fill_resource_security_addr(&addr);
    (void)nova_unlink(NOVA_RESOURCE_BROKER_AUTHORITY_SOCKET_PATH);
    long fd=nova_socket(NOVA_AF_UNIX,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK,0);
    if(fd<0){ log_simple("ERROR","resource_security","socket_failed","resource-broker principal authority socket creation failed"); return -1; }
    long rc=nova_bind((int)fd,&addr,sizeof(addr));
    if(rc<0){ nova_close((int)fd); log_simple("ERROR","resource_security","bind_failed","resource-broker principal authority bind failed"); return -1; }
    rc=nova_listen((int)fd,1);
    if(rc<0){ nova_close((int)fd); (void)nova_unlink(NOVA_RESOURCE_BROKER_AUTHORITY_SOCKET_PATH); log_simple("ERROR","resource_security","listen_failed","resource-broker principal authority listen failed"); return -1; }
    log_simple("INFO","resource_security","listening","broker-only live-principal authority is listening");
    return (int)fd;
}

static int app_security_request_valid(const struct nova_app_security_request *request) {
    if(!request) return 0;
    if(request->magic!=NOVA_APP_SECURITY_PROTOCOL_MAGIC ||
       request->version!=NOVA_APP_SECURITY_PROTOCOL_VERSION ||
       request->size!=(unsigned int)sizeof(*request) ||
       request->reserved0!=0U || request->reserved1!=0U) return 0;
    if(request->opcode==NOVA_APP_SECURITY_OP_HELLO) return request->process_id==0U;
    if(request->opcode==NOVA_APP_SECURITY_OP_LOOKUP_PID) return request->process_id!=0U;
    return 0;
}

static void fill_app_security_reply(struct nova_app_security_reply *reply,
                                    const struct nova_app_security_request *request,
                                    unsigned int opcode,unsigned int result,
                                    const struct nova_service *svc) {
    nova_zero(reply,sizeof(*reply));
    reply->magic=NOVA_APP_SECURITY_PROTOCOL_MAGIC;
    reply->version=NOVA_APP_SECURITY_PROTOCOL_VERSION;
    reply->opcode=opcode;
    reply->size=(unsigned int)sizeof(*reply);
    reply->request_id=request?request->request_id:0U;
    reply->result=result;
    reply->policy_generation=1ULL;
    if(svc){
        reply->process_id=(unsigned int)svc->pid;
        reply->client_id=svc->client_id;
        reply->security_flags=NOVA_APP_SECURITY_FLAG_AUTHENTICATED|NOVA_APP_SECURITY_FLAG_POLICY_BOUND;
        reply->app_generation=svc->app_generation;
        reply->declared_capabilities=svc->declared_capabilities;
        reply->effective_capabilities=svc->effective_capabilities;
        copy_string(reply->app_id,sizeof(reply->app_id),svc->app_id);
    }
}

static int process_app_security_packet(int fd) {
    struct nova_app_security_request request;
    struct nova_iovec iov;
    struct nova_msghdr msg;
    nova_zero(&request,sizeof(request)); nova_zero(&iov,sizeof(iov)); nova_zero(&msg,sizeof(msg));
    iov.iov_base=&request; iov.iov_len=sizeof(request); msg.msg_iov=&iov; msg.msg_iovlen=1;
    long n=nova_recvmsg(fd,&msg,0);
    if(n==-NOVA_EAGAIN || n==-NOVA_EINTR) return 0;
    if(n<=0) return -1;

    struct nova_app_security_reply reply;
    if(n!=(long)sizeof(request) || (msg.msg_flags&(NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC)) || !app_security_request_valid(&request)){
        fill_app_security_reply(&reply,&request,NOVA_APP_SECURITY_OP_ERROR,NOVA_APP_SECURITY_RESULT_BAD_REQUEST,(const struct nova_service *)0);
        return nova_write(fd,&reply,sizeof(reply))==(long)sizeof(reply) ? 0 : -1;
    }
    if(!normal_boot_policy_satisfied()){
        fill_app_security_reply(&reply,&request,NOVA_APP_SECURITY_OP_ERROR,NOVA_APP_SECURITY_RESULT_DENIED,(const struct nova_service *)0);
        return nova_write(fd,&reply,sizeof(reply))==(long)sizeof(reply) ? 0 : -1;
    }
    if(request.opcode==NOVA_APP_SECURITY_OP_HELLO){
        fill_app_security_reply(&reply,&request,NOVA_APP_SECURITY_OP_HELLO_REPLY,NOVA_APP_SECURITY_RESULT_OK,(const struct nova_service *)0);
        return nova_write(fd,&reply,sizeof(reply))==(long)sizeof(reply) ? 0 : -1;
    }
    struct nova_service *svc=find_application_by_pid(request.process_id);
    if(!svc || svc->app_generation==0ULL || !application_capability_policy_valid(svc)){
        fill_app_security_reply(&reply,&request,NOVA_APP_SECURITY_OP_ERROR,NOVA_APP_SECURITY_RESULT_NOT_FOUND,(const struct nova_service *)0);
    } else {
        fill_app_security_reply(&reply,&request,NOVA_APP_SECURITY_OP_PRINCIPAL,NOVA_APP_SECURITY_RESULT_OK,svc);
    }
    return nova_write(fd,&reply,sizeof(reply))==(long)sizeof(reply) ? 0 : -1;
}

static void disconnect_app_security_client(int *client_fd) {
    if(!client_fd || *client_fd<0) return;
    nova_close(*client_fd); *client_fd=-1;
    log_simple("INFO","app_security","client_disconnected","trusted application-security client disconnected");
}

static void accept_app_security_client(int listener,int *client_fd) {
    if(listener<0 || !client_fd) return;
    long fd=nova_accept4(listener,NOVA_SOCK_NONBLOCK);
    if(fd<0) return;
    unsigned int peer_pid=0U;
    struct nova_service *compositor=find_service("compositor");
    if(!peer_pid_for_socket((int)fd,&peer_pid) || !compositor || compositor->pid<=0 ||
       (unsigned long)compositor->pid!=(unsigned long)peer_pid){
        nova_close((int)fd);
        log_simple("WARN","app_security","client_rejected","application security client is not the supervised compositor");
        return;
    }
    if(*client_fd>=0){ nova_close((int)fd); return; }
    *client_fd=(int)fd;
    log_simple("INFO","app_security","client_connected","supervised compositor connected to application security authority");
}

static void disconnect_resource_security_client(int *client_fd) {
    if(!client_fd || *client_fd<0) return;
    nova_close(*client_fd); *client_fd=-1;
    log_simple("INFO","resource_security","client_disconnected","supervised resource broker disconnected from principal authority");
}

static void accept_resource_security_client(int listener,int *client_fd) {
    if(listener<0 || !client_fd) return;
    long fd=nova_accept4(listener,NOVA_SOCK_NONBLOCK);
    if(fd<0) return;
    unsigned int peer_pid=0U;
    struct nova_service *broker=find_service("resource-broker");
    if(!peer_pid_for_socket((int)fd,&peer_pid) || !broker || broker->pid<=0 ||
       (unsigned long)broker->pid!=(unsigned long)peer_pid){
        nova_close((int)fd);
        log_simple("WARN","resource_security","client_rejected","resource principal client is not the supervised resource-broker");
        return;
    }
    if(*client_fd>=0){ nova_close((int)fd); return; }
    *client_fd=(int)fd;
    log_simple("INFO","resource_security","client_connected","supervised resource broker connected to live-principal authority");
}

static void run_novad_event_loop(int app_listener,int catalog_listener) {
    int app_client=-1;
    int catalog_client=-1;
    int security_client=-1;
    int resource_security_client=-1;
    int overview_listener=setup_overview_listener(),overview_client=-1;
    int power_listener=setup_power_listener();
    char line[INPUT_MAX];
    unsigned long line_len=0;
    nova_zero(line,sizeof(line));
    nova_puts("nova> ");
    for(;;){
        reap_services();
        poll_overview(overview_listener,&overview_client);
        poll_power(power_listener);if(g_power_pending){log_simple("INFO","shutdown","desktop_request","authenticated compositor requested ordered poweroff");ordered_shutdown(NOVA_RB_POWER_OFF);g_power_pending=0;}
        struct nova_pollfd fds[9];
        nova_zero(fds,sizeof(fds));
        fds[0].fd=NOVA_STDIN; fds[0].events=NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP;
        fds[1].fd=app_listener; fds[1].events=app_listener>=0?NOVA_POLLIN:0;
        fds[2].fd=app_client; fds[2].events=app_client>=0?(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP):0;
        fds[3].fd=catalog_listener; fds[3].events=catalog_listener>=0?NOVA_POLLIN:0;
        fds[4].fd=catalog_client; fds[4].events=catalog_client>=0?(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP):0;
        /* Authority requests must wake PID 1 immediately, rather than wait for
         * the unrelated console/lifecycle 100 ms maintenance timeout. Keep the
         * same authenticated handlers and bounded one-packet dispatch below. */
        fds[5].fd=g_app_security_listener; fds[5].events=g_app_security_listener>=0?NOVA_POLLIN:0;
        fds[6].fd=security_client; fds[6].events=security_client>=0?(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP):0;
        fds[7].fd=g_resource_security_listener; fds[7].events=g_resource_security_listener>=0?NOVA_POLLIN:0;
        fds[8].fd=resource_security_client; fds[8].events=resource_security_client>=0?(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP):0;
        struct nova_timespec timeout={0,100000000L};
        long rc=nova_poll(fds,9,&timeout);
        if(rc<0 && rc!=-NOVA_EINTR){ log_simple("WARN","event_loop","poll_failed","novad event-loop poll failed"); continue; }
        if(app_client>=0){
            if(fds[2].revents&(NOVA_POLLERR|NOVA_POLLHUP)) disconnect_app_lifecycle_client(&app_client);
            else if((fds[2].revents&NOVA_POLLIN) && process_app_lifecycle_packet(app_client)<0) disconnect_app_lifecycle_client(&app_client);
        }
        if(catalog_client>=0){
            if(fds[4].revents&(NOVA_POLLERR|NOVA_POLLHUP)) disconnect_app_catalog_client(&catalog_client);
            else if((fds[4].revents&NOVA_POLLIN) && process_app_catalog_packet(catalog_client)<0) disconnect_app_catalog_client(&catalog_client);
        }
        if(app_listener>=0 && (fds[1].revents&NOVA_POLLIN)) accept_app_lifecycle_client(app_listener,&app_client);
        if(catalog_listener>=0 && (fds[3].revents&NOVA_POLLIN)) accept_app_catalog_client(catalog_listener,&catalog_client);

        struct nova_pollfd security_fds[2];
        nova_zero(security_fds,sizeof(security_fds));
        security_fds[0].fd=g_app_security_listener;
        security_fds[0].events=g_app_security_listener>=0?NOVA_POLLIN:0;
        security_fds[1].fd=security_client;
        security_fds[1].events=security_client>=0?(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP):0;
        security_fds[0].revents=fds[5].revents; security_fds[1].revents=fds[6].revents;
        long security_rc=rc;
        if(security_rc>=0){
            if(security_client>=0){
                if(security_fds[1].revents&(NOVA_POLLERR|NOVA_POLLHUP)) disconnect_app_security_client(&security_client);
                else if((security_fds[1].revents&NOVA_POLLIN) && process_app_security_packet(security_client)<0) disconnect_app_security_client(&security_client);
            }
            if(g_app_security_listener>=0 && (security_fds[0].revents&NOVA_POLLIN))
                accept_app_security_client(g_app_security_listener,&security_client);
        }

        struct nova_pollfd resource_security_fds[2];
        nova_zero(resource_security_fds,sizeof(resource_security_fds));
        resource_security_fds[0].fd=g_resource_security_listener;
        resource_security_fds[0].events=g_resource_security_listener>=0?NOVA_POLLIN:0;
        resource_security_fds[1].fd=resource_security_client;
        resource_security_fds[1].events=resource_security_client>=0?(NOVA_POLLIN|NOVA_POLLERR|NOVA_POLLHUP):0;
        resource_security_fds[0].revents=fds[7].revents; resource_security_fds[1].revents=fds[8].revents;
        long resource_security_rc=rc;
        if(resource_security_rc>=0){
            if(resource_security_client>=0){
                if(resource_security_fds[1].revents&(NOVA_POLLERR|NOVA_POLLHUP)) disconnect_resource_security_client(&resource_security_client);
                else if((resource_security_fds[1].revents&NOVA_POLLIN) && process_app_security_packet(resource_security_client)<0) disconnect_resource_security_client(&resource_security_client);
            }
            if(g_resource_security_listener>=0 && (resource_security_fds[0].revents&NOVA_POLLIN))
                accept_resource_security_client(g_resource_security_listener,&resource_security_client);
        }
        if(fds[0].revents&NOVA_POLLIN){ if(!console_consume_char(line,&line_len,sizeof(line))) break; }
        reap_services();
        enforce_application_stop_deadlines();
    }
    if(g_power_client>=0){nova_close(g_power_client);g_power_client=-1;}if(power_listener>=0){nova_close(power_listener);(void)nova_unlink(NOVA_POWER_PATH);}
    if(overview_client>=0)nova_close(overview_client);if(overview_listener>=0){nova_close(overview_listener);(void)nova_unlink(NOVA_OVERVIEW_PATH);}
    disconnect_app_lifecycle_client(&app_client);
    disconnect_app_catalog_client(&catalog_client);
    disconnect_app_security_client(&security_client);
    disconnect_resource_security_client(&resource_security_client);
}

static void prepare_test_network(void){
 if(nova_getpid()!=1||!normal_boot_policy_satisfied())return;
 long pid=nova_clone_process();if(pid<0)return;
 if(!pid){char *argv[]={"/System/Core/nova-net-setup",(char*)0};char *env[]={(char*)0};nova_execve(argv[0],argv,env);nova_exit(126);}
 int status=0;long got;do{got=nova_wait4(pid,&status,0);}while(got==-4);
 if(got==pid&&status==0)log_simple("INFO","network","test_nat_ready","signed opt-in static NAT TEST profile; not DHCP or TLS readiness");
 else log_simple("WARN","network","test_nat_unavailable","not enabled or interface preparation failed; no network readiness claim");
}

static void prepare_user_workspace(void){
 if(nova_getpid()!=1||!normal_boot_policy_satisfied())return;
 long pid=nova_clone_process();if(pid<0){log_simple("ERROR","workspace","spawn_failed","shared home unavailable");return;}
 if(!pid){char *argv[]={"/System/Core/nova-user-workspace",(char*)0};char *env[]={(char*)0};nova_execve(argv[0],argv,env);nova_exit(126);}
 int status=0;long got;do{got=nova_wait4(pid,&status,0);}while(got==-4);
 if(got==pid&&status==0){g_user_workspace_ready=1;log_simple("INFO","workspace","shared_home_ready","terminal-rw / Files private read-only worker; app uid remains 3");}
 else log_simple("WARN","workspace","shared_home_unavailable","not provisioned or preparation failed; no shared home grant");
}

static void start_enabled_services(void) {
    g_boot_stage="SERVICE_START";
    if(!normal_boot_policy_satisfied()){
        log_simple("WARN","services","start_blocked","services blocked because the unified normal-boot authorization policy is not satisfied");
        return;
    }
    for(int i=0;i<g_service_count;i++) if(g_services[i].enabled && g_services[i].autostart) start_service(&g_services[i]);
}

static void stop_service(struct nova_service *svc) {
    if(!svc || svc->pid<=0){ if(svc){ svc->desired_running=0; if(svc->is_application) svc->app_state=NOVA_APP_STATE_STOPPED; } return; }
    long pid=svc->pid; svc->desired_running=0; if(svc->is_application) svc->app_state=NOVA_APP_STATE_STOPPING;
    (void)nova_kill(pid,NOVA_SIGTERM); int status=0; (void)nova_wait4(pid,&status,0);
    log_service("INFO","stopped",svc,status); svc->pid=0; if(svc->is_application){ svc->app_state=NOVA_APP_STATE_STOPPED; write_app_lifecycle_state(); }
}

static void shutdown_services(void) {
    g_boot_stage="SHUTDOWN"; log_simple("INFO","shutdown","stopping_services","stopping services");
    for(int i=g_service_count-1;i>=0;i--) stop_service(&g_services[i]);
}

static void write_boot_state(void) {
    if(nova_getpid()!=1) return;
    long fd=nova_open_write("/run/nova/boot.state",0644); if(fd<0) return;
    nova_fd_puts((int)fd,"version="); nova_fd_putln((int)fd,NOVA_VERSION);
    nova_fd_puts((int)fd,"stage="); nova_fd_putln((int)fd,g_boot_stage);
    nova_fd_puts((int)fd,"health="); nova_fd_putln((int)fd,g_boot_degraded?"DEGRADED":"OK");
    nova_fd_puts((int)fd,"firmware_trust="); nova_fd_putln((int)fd,firmware_trust_name());
    nova_fd_puts((int)fd,"firmware_uefi="); nova_fd_putln((int)fd,g_firmware_uefi?"yes":"no");
    nova_fd_puts((int)fd,"firmware_secure_boot="); nova_fd_putln((int)fd,g_firmware_secure_boot?"yes":"no");
    nova_fd_puts((int)fd,"firmware_efivarfs="); nova_fd_putln((int)fd,g_efivarfs_active?"yes":"no");
    nova_fd_puts((int)fd,"firmware_setup_mode=");
    if(g_firmware_setup_mode<0) nova_fd_putln((int)fd,"unknown"); else nova_fd_putln((int)fd,g_firmware_setup_mode?"yes":"no");
    nova_fd_puts((int)fd,"uefi_signer_cert_sha256="); nova_fd_putln((int)fd,NOVA_UEFI_CERT_SHA256);
    nova_fd_putln((int)fd,"initramfs_binding=builtin-in-signed-efi-kernel");
    nova_fd_puts((int)fd,"trust="); nova_fd_putln((int)fd,trust_name());
    nova_fd_puts((int)fd,"trust_profile="); nova_fd_putln((int)fd,NOVA_TRUST_PROFILE);
    nova_fd_puts((int)fd,"trust_key_id="); nova_fd_putln((int)fd,NOVA_TRUST_KEY_ID);
    nova_fd_puts((int)fd,"release_policy_verified="); nova_fd_putln((int)fd,g_release_policy_verified?"yes":"no");
    nova_fd_puts((int)fd,"release_policy_format="); nova_fd_put_u64((int)fd,(unsigned long)g_release_policy_format); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"release_policy_state="); nova_fd_putln((int)fd,g_release_policy_state);
    nova_fd_puts((int)fd,"release_policy_sha256="); nova_fd_putln((int)fd,g_release_policy_sha256[0]?g_release_policy_sha256:"unavailable");
    nova_fd_puts((int)fd,"preboot_release_policy_verified="); nova_fd_putln((int)fd,g_preboot_release_policy_verified?"yes":"no");
    nova_fd_puts((int)fd,"preboot_release_policy_sha256="); nova_fd_putln((int)fd,g_preboot_release_policy_sha256[0]?g_preboot_release_policy_sha256:"unavailable");
    nova_fd_puts((int)fd,"release_trust_epoch="); nova_fd_put_u64((int)fd,g_release_trust_epoch); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"release_trust_epoch_state="); nova_fd_putln((int)fd,release_epoch_state(g_release_trust_epoch));
    nova_fd_puts((int)fd,"release_active_key_id="); nova_fd_putln((int)fd,g_release_active_key_id);
    nova_fd_puts((int)fd,"release_active_cert_sha256="); nova_fd_putln((int)fd,g_release_active_cert_sha256[0]?g_release_active_cert_sha256:"unavailable");
    nova_fd_puts((int)fd,"release_next_key_id="); nova_fd_putln((int)fd,g_release_next_key_id);
    nova_fd_puts((int)fd,"release_next_cert_sha256="); nova_fd_putln((int)fd,g_release_next_cert_sha256[0]?g_release_next_cert_sha256:"unavailable");
    nova_fd_puts((int)fd,"release_revoked_key_ids="); nova_fd_putln((int)fd,g_release_revoked_key_ids);
    nova_fd_puts((int)fd,"integrity="); nova_fd_putln((int)fd,integrity_name());
    nova_fd_puts((int)fd,"system_readonly="); nova_fd_putln((int)fd,g_system_readonly?"yes":"no");
    nova_fd_puts((int)fd,"recovery_readonly="); nova_fd_putln((int)fd,g_recovery_readonly?"yes":"no");
    nova_fd_puts((int)fd,"recovery_payload_trust="); nova_fd_putln((int)fd,recovery_payload_verified()?"TRUSTED":"NOT_TRUSTED");
    nova_fd_puts((int)fd,"persistent_mutation_authorized="); nova_fd_putln((int)fd,mutation_health_ready()?"yes":"no");
    nova_fd_puts((int)fd,"data_writable="); nova_fd_putln((int)fd,g_data_writable?"yes":"no");
    nova_fd_puts((int)fd,"storage_persistent="); nova_fd_putln((int)fd,g_storage_persistent?"yes":"no");
    nova_fd_puts((int)fd,"storage_ready="); nova_fd_putln((int)fd,g_storage_ready?"yes":"no");
    nova_fd_puts((int)fd,"verity="); nova_fd_putln((int)fd,verity_name());
    nova_fd_puts((int)fd,"verity_mapping_active="); nova_fd_putln((int)fd,g_verity_mapping_active?"yes":"no");
    nova_fd_puts((int)fd,"verity_descriptor_verified="); nova_fd_putln((int)fd,g_verity_descriptor_verified?"yes":"no");
    nova_fd_puts((int)fd,"verity_rollback_scheduled="); nova_fd_putln((int)fd,g_verity_rollback_scheduled?"yes":"no");
    nova_fd_puts((int)fd,"verity_root_hash="); nova_fd_putln((int)fd,g_verity_root_hash[0]?g_verity_root_hash:"none");
    nova_fd_puts((int)fd,"recovery_verity="); nova_fd_putln((int)fd,recovery_verity_name());
    nova_fd_puts((int)fd,"recovery_verity_mapping_active="); nova_fd_putln((int)fd,g_recovery_verity_mapping_active?"yes":"no");
    nova_fd_puts((int)fd,"recovery_verity_descriptor_verified="); nova_fd_putln((int)fd,g_recovery_verity_descriptor_verified?"yes":"no");
    nova_fd_puts((int)fd,"recovery_descriptor_source="); nova_fd_putln((int)fd,g_recovery_descriptor_source);
    nova_fd_puts((int)fd,"recovery_verity_root_hash="); nova_fd_putln((int)fd,g_recovery_verity_root_hash[0]?g_recovery_verity_root_hash:"none");
    nova_fd_puts((int)fd,"tpm_state="); nova_fd_putln((int)fd,tpm_state_name());
    nova_fd_puts((int)fd,"tpm_device_node="); nova_fd_putln((int)fd,nova_tpm2_device_node_present()?"present":"missing");
    nova_fd_puts((int)fd,"tpm_pcr_read_source="); nova_fd_putln((int)fd,nova_tpm2_last_pcr_source());
    nova_fd_puts((int)fd,"tpm_last_rc="); nova_fd_put_u64((int)fd,nova_tpm2_last_rc()); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"tpm_last_rc_name="); nova_fd_putln((int)fd,nova_tpm2_rc_name(nova_tpm2_last_rc()));
    nova_fd_puts((int)fd,"tpm_counter_initialized="); nova_fd_putln((int)fd,nova_tpm2_counter_initialized()?"yes":"no");
    nova_fd_puts((int)fd,"tpm_raw_counter="); nova_fd_put_u64((int)fd,nova_tpm2_last_raw_counter()); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"tpm_counter_base="); nova_fd_put_u64((int)fd,nova_tpm2_last_counter_base()); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"tpm_baseline_written="); nova_fd_putln((int)fd,nova_tpm2_baseline_written()?"yes":"no");
    nova_fd_puts((int)fd,"tpm_baseline_locked="); nova_fd_putln((int)fd,nova_tpm2_baseline_locked()?"yes":"no");
    nova_fd_puts((int)fd,"measurement_failed="); nova_fd_putln((int)fd,g_measurement_failed?"yes":"no");
    nova_fd_puts((int)fd,"evidence_error="); nova_fd_putln((int)fd,g_evidence_error);
    nova_fd_puts((int)fd,"active_slot_descriptor_format="); nova_fd_put_u64((int)fd,(unsigned long)g_active_slot_descriptor_format); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"active_slot_descriptor_kind="); nova_fd_putln((int)fd,g_active_slot_descriptor_kind);
    nova_fd_putln((int)fd,"tpm_nv_index=0x01504e56");
    nova_fd_putln((int)fd,"tpm_nv_baseline_index=0x01504e57");
    nova_fd_putln((int)fd,"tpm_trust_epoch_nv_index=0x01504e58");
    nova_fd_putln((int)fd,"tpm_trust_epoch_baseline_index=0x01504e59");
    nova_fd_puts((int)fd,"tpm_trust_epoch_counter="); nova_fd_put_u64((int)fd,g_tpm_trust_epoch_counter); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"tpm_trust_counter_created="); nova_fd_putln((int)fd,g_tpm_trust_counter_created?"yes":"no");
    nova_fd_puts((int)fd,"tpm_trust_epoch_commit_recovered="); nova_fd_putln((int)fd,g_tpm_trust_epoch_commit_recovered?"yes":"no");
    nova_fd_puts((int)fd,"trust_epoch_rejected="); nova_fd_putln((int)fd,g_trust_epoch_rejected?"yes":"no");
    nova_fd_puts((int)fd,"tpm_rollback_counter="); nova_fd_put_u64((int)fd,g_tpm_rollback_counter); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"active_rollback_index="); nova_fd_put_u64((int)fd,g_active_rollback_index); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"tpm_counter_created="); nova_fd_putln((int)fd,g_tpm_counter_created?"yes":"no");
    nova_fd_puts((int)fd,"tpm_counter_commit_recovered="); nova_fd_putln((int)fd,g_tpm_counter_commit_recovered?"yes":"no");
    nova_fd_puts((int)fd,"antirollback_rejected="); nova_fd_putln((int)fd,g_antirollback_rejected?"yes":"no");
    nova_fd_puts((int)fd,"measured_boot="); nova_fd_putln((int)fd,g_measured_boot?"VERIFIED":"NOT VERIFIED");
    nova_fd_puts((int)fd,"event_log_verified="); nova_fd_putln((int)fd,g_event_log_verified?"yes":"no");
    nova_fd_puts((int)fd,"event_log_error="); nova_fd_putln((int)fd,g_event_log_error);
    nova_fd_puts((int)fd,"event_log_sha256="); nova_fd_putln((int)fd,g_event_log_sha256[0]?g_event_log_sha256:"unavailable");
    nova_fd_puts((int)fd,"event_log_replayed_pcr7="); nova_fd_putln((int)fd,g_event_log_replayed_pcr7[0]?g_event_log_replayed_pcr7:"unavailable");
    nova_fd_puts((int)fd,"event_log_bytes="); nova_fd_put_u64((int)fd,g_event_log_bytes); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"event_log_events="); nova_fd_put_u64((int)fd,g_event_log_events); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"event_log_pcr7_events="); nova_fd_put_u64((int)fd,g_event_log_pcr7_events); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"pcr7_sha256="); nova_fd_putln((int)fd,g_pcr7_sha256[0]?g_pcr7_sha256:"unavailable");
    nova_fd_puts((int)fd,"pcr7_source="); nova_fd_putln((int)fd,g_pcr7_source);
    nova_fd_puts((int)fd,"pcr14_before="); nova_fd_putln((int)fd,g_pcr14_before[0]?g_pcr14_before:"unavailable");
    nova_fd_puts((int)fd,"pcr14_before_source="); nova_fd_putln((int)fd,g_pcr14_before_source);
    nova_fd_puts((int)fd,"measurement_digest="); nova_fd_putln((int)fd,g_measurement_digest[0]?g_measurement_digest:"unavailable");
    nova_fd_puts((int)fd,"pcr14_after="); nova_fd_putln((int)fd,g_pcr14_after[0]?g_pcr14_after:"unavailable");
    nova_fd_puts((int)fd,"pcr14_after_source="); nova_fd_putln((int)fd,g_pcr14_after_source);
    nova_fd_putln((int)fd,"measurement_log=/run/nova/measurements.log");
    nova_fd_putln((int)fd,"evidence_file=/run/nova/boot-evidence.env");
    nova_fd_puts((int)fd,"active_slot="); char active[2]={g_active_slot,'\0'}; nova_fd_putln((int)fd,active);
    nova_fd_puts((int)fd,"next_slot="); char next[2]={g_next_slot,'\0'}; nova_fd_putln((int)fd,next);
    nova_fd_puts((int)fd,"slot_generation="); nova_fd_put_u64((int)fd,g_slot_generation); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"slot_confirmed="); nova_fd_putln((int)fd,g_slot_confirmed?"yes":"no");
    nova_fd_puts((int)fd,"boot_attempts_left="); nova_fd_put_u64((int)fd,g_boot_attempts_left); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"pending_kind="); nova_fd_putln((int)fd,g_pending_kind);
    nova_fd_puts((int)fd,"update_id="); nova_fd_putln((int)fd,g_update_id);
    nova_fd_puts((int)fd,"rollback_performed="); nova_fd_putln((int)fd,g_rollback_performed?"yes":"no");
    nova_fd_puts((int)fd,"slot_metadata_valid="); nova_fd_putln((int)fd,g_slot_metadata_valid?"yes":"no");
    nova_fd_puts((int)fd,"slot_metadata_seq="); nova_fd_put_u64((int)fd,g_slot_metadata_seq); nova_fd_putln((int)fd,"");
    nova_fd_puts((int)fd,"slot_metadata_source="); char source[2]={g_slot_metadata_source,'\0'}; nova_fd_putln((int)fd,source);
    nova_fd_puts((int)fd,"slot_metadata_recovered="); nova_fd_putln((int)fd,g_slot_metadata_recovered?"yes":"no");
    nova_fd_puts((int)fd,"slot_journal_write_failed="); nova_fd_putln((int)fd,g_slot_journal_write_failed?"yes":"no");
    nova_close((int)fd);
}

static void stream_file(const char *path) {
    char buffer[FILE_BUFFER];
    long fd=nova_open_ro(path);
    if(fd<0){ nova_puts("cat: cannot open "); nova_putln(path); return; }
    for(;;){
        long n=nova_read((int)fd,buffer,sizeof(buffer));
        if(n == -NOVA_EINTR) continue;
        if(n<=0) break;
        if(nova_fd_write_all(NOVA_STDOUT,buffer,(nova_size_t)n)<0) break;
    }
    nova_close((int)fd);
}

static int write_atomic_payload(const char *tmp, const char *path, const char *text) {
    long fd=nova_open_write(tmp,0644);
    if(fd<0) return 0;
    if(nova_fd_write_all((int)fd,text,nova_strlen(text))<0){nova_close((int)fd);return 0;}
    if(nova_fsync((int)fd)<0){nova_close((int)fd);return 0;}
    nova_close((int)fd);
    if(nova_rename(tmp,path)<0) return 0;
    nova_sync();
    return 1;
}

static void cmd_storage(void) {
    nova_putln("NovaOS Stage-4D verified persistent storage");
    nova_puts("  State         : "); nova_putln(g_storage_ready?"READY":"NOT READY");
    nova_puts("  Mode          : "); nova_putln(g_storage_persistent?"GPT / discovered block device":"initramfs fallback");
    nova_puts("  Raw System    : "); nova_putln(g_system_device);
    nova_puts("  Verified map  : "); nova_putln(g_verity_mapping_active?NOVA_DM_NODE:"<inactive>");
    nova_puts("  Raw Recovery  : "); nova_putln(NOVA_RECOVERY_DEV);
    nova_puts("  Recovery map  : "); nova_putln(g_recovery_verity_mapping_active?NOVA_RECOVERY_DM_NODE:"<inactive>");
    nova_puts("  Data          : "); nova_putln(NOVA_DATA_DEV);
    nova_puts("  Data writable : "); nova_putln(g_data_writable?"yes":"no");
    nova_putln("  Data flags    : rw,nosuid,nodev,noexec");
    nova_putln("  System flags  : ro,nosuid,nodev");
    nova_putln("  Recovery flags: ro,nosuid,nodev,noexec");
    nova_puts("  Running slot  : "); char a[2]={g_active_slot,'\0'}; nova_putln(a);
    nova_puts("  Next slot     : "); char n[2]={g_next_slot,'\0'}; nova_putln(n);
    nova_putln("  ESP           : 64 MiB FAT32 / signed EFI-stub boot payload");
}

static int recovery_payload_verified(void) {
    return g_firmware_trust==FIRMWARE_TRUST_VERIFIED && g_firmware_secure_boot &&
           g_recovery_verity_descriptor_verified && g_recovery_verity==VERITY_ENFORCED &&
           g_recovery_verity_mapping_active && g_recovery_readonly;
}

static void cmd_recovery(void) {
    nova_putln("NovaOS Stage-4D recovery trust policy");
    nova_puts("  Firmware root        : "); nova_putln(g_firmware_trust==FIRMWARE_TRUST_VERIFIED?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Recovery descriptor  : "); nova_putln(g_recovery_verity_descriptor_verified?"VERIFIED":"FAILED/UNAVAILABLE");
    nova_puts("  Descriptor source    : "); nova_putln(g_recovery_descriptor_source);
    nova_puts("  Recovery dm-verity   : "); nova_putln(g_recovery_verity==VERITY_ENFORCED?"ENFORCED":"NOT ENFORCED");
    nova_puts("  Recovery read-only   : "); nova_putln(g_recovery_readonly?"ENFORCED":"NOT ENFORCED");
    nova_puts("  Recovery payload     : "); nova_putln(recovery_payload_verified()?"TRUSTED":"NOT TRUSTED");
    nova_puts("  Normal boot          : "); nova_putln(normal_boot_policy_satisfied()?"AUTHORIZED":"REJECTED");
    nova_puts("  Persistent mutation  : "); nova_putln(mutation_health_ready()?"AUTHORIZED":"BLOCKED");
    nova_putln("  Data trust           : MUTABLE / UNTRUSTED DIAGNOSTIC INPUT");
    if(normal_boot_policy_satisfied())
        nova_putln("  Mode                 : NORMAL (Recovery remains independently verified and mounted read-only)");
    else if(recovery_payload_verified())
        nova_putln("  Mode                 : TRUSTED DIAGNOSTIC (normal services and persistent mutations blocked)");
    else
        nova_putln("  Mode                 : INITRAMFS DIAGNOSTIC ONLY (Recovery payload not authorized)");
    nova_putln("  Recovery trust does not override failed System, TPM, release-policy, or measured-boot authorization.");
}

static void cmd_verity(void) {
    nova_putln("NovaOS Stage-4D dm-verity state");
    nova_puts("  State         : "); nova_putln(verity_name());
    nova_puts("  Descriptor    : "); nova_putln(g_verity_descriptor_verified?"SIGNED / VERIFIED":"NOT VERIFIED");
    nova_puts("  Mapping       : "); nova_putln(g_verity_mapping_active?NOVA_DM_NODE:"<inactive>");
    nova_puts("  Raw slot      : "); nova_putln(g_system_device);
    nova_puts("  Root hash     : "); nova_putln(g_verity_root_hash[0]?g_verity_root_hash:"none");
    nova_puts("  Rollback index: "); nova_put_u64(g_active_rollback_index); nova_putln("");
    nova_puts("  Salt          : "); nova_putln(g_verity_salt[0]?g_verity_salt:"none");
    nova_puts("  Data blocks   : "); nova_put_u64(NOVA_VERITY_DATA_BLOCKS); nova_putln("");
    nova_puts("  Hash start    : "); nova_put_u64(NOVA_VERITY_HASH_START_BLOCK); nova_putln(" blocks");
    nova_puts("  Rollback sched: "); nova_putln(g_verity_rollback_scheduled?"yes":"no");
    nova_putln("  Failure policy: reject verified System mount; pending slot rollback is persisted for next boot");
    nova_putln("  Recovery:");
    nova_puts("    State       : "); nova_putln(recovery_verity_name());
    nova_puts("    Descriptor  : "); nova_putln(g_recovery_verity_descriptor_verified?"SIGNED / VERIFIED":"NOT VERIFIED");
    nova_puts("    Source      : "); nova_putln(g_recovery_descriptor_source);
    nova_puts("    Mapping     : "); nova_putln(g_recovery_verity_mapping_active?NOVA_RECOVERY_DM_NODE:"<inactive>");
    nova_puts("    Raw device  : "); nova_putln(NOVA_RECOVERY_DEV);
    nova_puts("    Root hash   : "); nova_putln(g_recovery_verity_root_hash[0]?g_recovery_verity_root_hash:"none");
    nova_puts("    Data blocks : "); nova_put_u64(NOVA_RECOVERY_VERITY_DATA_BLOCKS); nova_putln("");
    nova_putln("    Failure     : fail closed; trusted Recovery is required for Stage-4C READY");
}

static void cmd_slot(void) {
    nova_putln("NovaOS A/B slot foundation");
    nova_puts("  Running       : "); char a[2]={g_active_slot,'\0'}; nova_putln(a);
    nova_puts("  Next boot     : "); char n[2]={g_next_slot,'\0'}; nova_putln(n);
    nova_puts("  Previous      : "); char p[2]={g_previous_slot,'\0'}; nova_putln(p);
    nova_puts("  Generation    : "); nova_put_u64(g_slot_generation); nova_putln("");
    nova_puts("  Confirmed     : "); nova_putln(g_slot_confirmed?"yes":"no");
    nova_puts("  Attempts left : "); nova_put_u64(g_boot_attempts_left); nova_putln("");
    nova_puts("  Pending kind  : "); nova_putln(g_pending_kind);
    nova_puts("  Update ID     : "); nova_putln(g_update_id);
    nova_puts("  Journal seq   : "); nova_put_u64(g_slot_metadata_seq); nova_putln("");
    nova_puts("  Journal source: "); char js[2]={g_slot_metadata_source,'\0'}; nova_putln(js);
    nova_puts("  Journal mirror: "); nova_putln(g_slot_metadata_recovered?"RECOVERED / SELF-HEALED":"REDUNDANT / HEALTHY");
    nova_puts("  Metadata check: "); nova_putln(g_slot_metadata_valid?"DUAL SHA-256 RECORDS":"SAFE STATE REBUILT");
    nova_puts("  Journal writes: "); nova_putln(g_slot_journal_write_failed?"FAILED / NORMAL BOOT BLOCKED":"HEALTHY");
    nova_putln("  Metadata auth : UNAUTHENTICATED; redundancy + checksum protects corruption/power-loss, not hostile rollback");
    nova_putln("  Rollback      : AUTOMATIC after pending attempt budget is exhausted");
    nova_puts("  TPM counter   : "); nova_put_u64(g_tpm_rollback_counter); nova_putln("");
    nova_puts("  Slot rb index : "); nova_put_u64(g_active_rollback_index); nova_putln("");
    nova_putln("  Anti-rollback : TPM monotonic counter + signed per-slot rollback_index");
}

static void slot_action(const char *args) {
    args=nova_skip_spaces(args);
    if(nova_streq(args,"confirm")){
        if(!g_storage_persistent){nova_putln("slot: persistent storage unavailable");return;}
        if(!normal_boot_policy_satisfied()){
            nova_putln("slot: confirm refused; unified normal-boot authorization policy is not satisfied"); return;
        }
        unsigned long pending_rollback=0,committed_epoch=0,pending_epoch=0;
        if(!checked_add_ul(g_tpm_rollback_counter,1UL,&pending_rollback) ||
           g_active_rollback_index<g_tpm_rollback_counter || g_active_rollback_index>pending_rollback){
            nova_putln("slot: confirm refused; rollback index is inconsistent with TPM monotonic counter"); return;
        }
        if(!release_epoch_bounds(&committed_epoch,&pending_epoch) ||
           g_release_trust_epoch<committed_epoch || g_release_trust_epoch>pending_epoch){
            nova_putln("slot: confirm refused; release trust epoch is inconsistent with TPM monotonic trust state"); return;
        }
        /* Power-loss ordering: journal confirmation is durable first. Each TPM
         * counter commit is idempotently recovered on the next verified boot. */
        if(!commit_slot_metadata(g_active_slot,g_previous_slot,g_slot_generation,1,0,"none",g_update_id)){nova_putln("slot: metadata write failed");return;}
        g_next_slot=g_active_slot; g_slot_confirmed=1; g_boot_attempts_left=0; copy_string(g_pending_kind,sizeof(g_pending_kind),"none");
        if(g_active_rollback_index==pending_rollback){
            unsigned long new_value=0;
            if(!nova_tpm2_increment_rollback_counter(g_tpm_rollback_counter,&new_value) || new_value!=g_active_rollback_index){
                g_tpm_state=TPM_STATE_FAILED; g_boot_degraded=1; write_boot_state();
                nova_putln("slot: journal confirmed but TPM counter commit failed; next verified boot must recover commit"); return;
            }
            g_tpm_rollback_counter=new_value;
        }
        if(!release_epoch_bounds(&committed_epoch,&pending_epoch)){
            g_tpm_state=TPM_STATE_FAILED; g_boot_degraded=1; write_boot_state();
            nova_putln("slot: trust-epoch counter space exhausted or unavailable"); return;
        }
        if(g_release_trust_epoch==pending_epoch){
            unsigned long new_value=0,new_committed_epoch=0;
            if(!nova_tpm2_increment_trust_epoch_counter(g_tpm_trust_epoch_counter,&new_value) ||
               !checked_add_ul(new_value,1UL,&new_committed_epoch) || new_committed_epoch!=g_release_trust_epoch){
                g_tpm_state=TPM_STATE_FAILED; g_boot_degraded=1; write_boot_state();
                nova_putln("slot: journal confirmed but TPM trust-epoch commit failed; next verified boot must recover commit"); return;
            }
            g_tpm_trust_epoch_counter=new_value;
        }
        write_boot_state();
        nova_putln("slot: running slot confirmed; TPM rollback/trust anti-rollback state committed");
        return;
    }
    if(nova_startswith(args,"set ")){
        const char *v=nova_skip_spaces(args+4);
        if(!((v[0]=='A'||v[0]=='B') && v[1]=='\0')){nova_putln("usage: slot set <A|B>");return;}
        if(!g_storage_persistent){nova_putln("slot: persistent storage unavailable");return;}
        if(!mutation_health_ready()){nova_putln("slot: change refused because firmware/System/Recovery/TPM measured-boot health is not fully verified");return;}
        char target=v[0];
        struct nova_verity_descriptor target_desc;
        if(!verify_slot_verity_descriptor_ex(target,&target_desc,0)){nova_putln("slot: target signed descriptor invalid");return;}
        unsigned long target_max_rollback=0;
        if(!checked_add_ul(g_tpm_rollback_counter,1UL,&target_max_rollback) ||
           target_desc.rollback_index<g_tpm_rollback_counter || target_desc.rollback_index>target_max_rollback){
            nova_putln("slot: target rejected by TPM anti-rollback policy");return;
        }
        unsigned long next_generation=0;
        if(!checked_add_ul(g_slot_generation,1UL,&next_generation) ||
           !commit_slot_metadata(target,g_active_slot,next_generation,0,1,"manual","none")){nova_putln("slot: metadata generation exhausted or write failed");return;}
        g_previous_slot=g_active_slot; g_next_slot=target; g_slot_generation=next_generation; g_slot_confirmed=0; g_boot_attempts_left=1; copy_string(g_pending_kind,sizeof(g_pending_kind),"manual"); copy_string(g_update_id,sizeof(g_update_id),"none");
        write_boot_state();
        nova_puts("slot: next boot set to "); nova_putln(v);
        nova_putln("slot: change takes effect after reboot; selected slot must still pass trust/integrity checks");
        return;
    }
    nova_putln("usage: slot [set <A|B>|confirm]");
}

static void persist_action(const char *args) {
    args=nova_skip_spaces(args);
    if(nova_streq(args,"show")){
        if(!g_storage_persistent){nova_putln("persist: persistent Data is unavailable");return;}
        long fd=nova_open_ro(NOVA_PERSIST_FILE);
        if(fd<0){nova_putln("persist: <unset>");return;}
        nova_close((int)fd); stream_file(NOVA_PERSIST_FILE); return;
    }
    if(nova_streq(args,"clear")){
        if(!g_storage_persistent){nova_putln("persist: persistent Data is unavailable");return;}
        if(!mutation_health_ready()){nova_putln("persist: mutation refused outside fully authorized normal boot");return;}
        (void)nova_unlink(NOVA_PERSIST_FILE); nova_sync(); nova_putln("persist: cleared"); return;
    }
    if(nova_startswith(args,"write ")){
        const char *text=nova_skip_spaces(args+6);
        if(!*text){nova_putln("usage: persist write <text>");return;}
        if(!g_storage_persistent){nova_putln("persist: persistent Data is unavailable");return;}
        if(!mutation_health_ready()){nova_putln("persist: mutation refused outside fully authorized normal boot");return;}
        if(!write_atomic_payload(NOVA_PERSIST_TMP,NOVA_PERSIST_FILE,text)){nova_putln("persist: write failed");return;}
        nova_putln("persist: value committed to /Data"); return;
    }
    nova_putln("usage: persist <show|write <text>|clear>");
}

static void banner(void) {
    nova_putln("");
    nova_putln("  _   _                 ___  ____  ");
    nova_putln(" | \\ | | _____   ____ _/ _ \\/ ___| ");
    nova_putln(" |  \\| |/ _ \\ \\ / / _` | | | \\___ \\ ");
    nova_putln(" | |\\  | (_) \\ V / (_| | |_| |___) |");
    nova_putln(" |_| \\_|\\___/ \\_/ \\__,_|\\___/|____/ ");
    nova_putln("");
    nova_puts(" NovaOS Stage-4D "); nova_putln(NOVA_VERSION);
    nova_puts(" Architecture    "); nova_putln(NOVA_BUILD_ARCH);
    nova_putln(" Userspace       Nova native / freestanding (no libc)");
    nova_putln(" Init/Supervisor novad (PID 1)");
    nova_putln(" System model    Secure Boot + TPM anti-rollback + event-log evidence + delegated release trust + dm-verity A/B/Recovery");
    nova_putln(" Kernel          Linux (custom NovaOS configuration)"); nova_putln("");
}

static void cmd_help(void) {
    nova_putln("Nova shell built-ins:");
    nova_putln("  help                        Show this help");
    nova_putln("  version                     NovaOS userspace version");
    nova_putln("  boot                        novad boot/supervisor state");
    nova_putln("  firmware                    UEFI Secure Boot trust state");
    nova_putln("  tpm                         TPM anti-rollback / measured-boot state");
    nova_putln("  evidence                    Measured-boot evidence state");
    nova_putln("  state                       Machine-readable boot state");
    nova_putln("  layout                      System/Data/Recovery policy");
    nova_putln("  storage                     Persistent GPT storage state");
    nova_putln("  recovery                    Recovery trust / diagnostic policy");
    nova_putln("  slot                        A/B slot state");
    nova_putln("  slot set <A|B>              Select next boot slot");
    nova_putln("  slot confirm                Confirm the running slot");
    nova_putln("  update status               Signed update / rollback state");
    nova_putln("  update verify               Verify staged update bundle");
    nova_putln("  update apply                Write verified update to inactive slot");
    nova_putln("  persist show                Read persistence test value");
    nova_putln("  persist write <text>        Persist a value across reboot");
    nova_putln("  persist clear               Remove persistence test value");
    nova_putln("  verity                      dm-verity System + Recovery mapping state");
    nova_putln("  trust                       Signed-manifest trust state");
    nova_putln("  integrity                   Runtime integrity result");
    nova_putln("  selftest                    Core cryptographic primitive self-test");
    nova_putln("  services                    List supervised services");
    nova_putln("  service start <name>        Start a service");
    nova_putln("  service stop <name>         Stop a service");
    nova_putln("  service restart <name>      Restart a service");
    nova_putln("  service kill <name>         Crash-test restart policy");
    nova_putln("  logs                        Show structured novad runtime log");
    nova_putln("  uname                       Kernel/system identity");
    nova_putln("  mem                         Memory and uptime");
    nova_putln("  security                    Stage-4D security state");
    nova_putln("  mounts                      Show /proc/mounts");
    nova_putln("  cat <path>                  Read a text file");
    nova_putln("  echo <text>                 Print text");
    nova_putln("  pwd                         Show current directory");
    nova_putln("  clear                       Clear terminal");
    nova_putln("  reboot                      Ordered reboot (PID 1 only)");
    nova_putln("  poweroff                    Ordered power off (PID 1 only)");
    nova_putln("  exit                        Exit only outside PID 1");
}

static void cmd_version(void){ nova_puts("NovaOS "); nova_puts(NOVA_VERSION); nova_puts(" ["); nova_puts(NOVA_BUILD_ARCH); nova_putln("]"); }

static void cmd_boot(void) {
    nova_putln("novad supervisor state");
    nova_puts("  PID                  : "); nova_put_u64((unsigned long)nova_getpid()); nova_putln("");
    nova_puts("  Boot stage           : "); nova_putln(g_boot_stage);
    nova_puts("  Firmware trust       : "); nova_putln(firmware_trust_name());
    nova_puts("  TPM state            : "); nova_putln(tpm_state_name());
    nova_puts("  TPM rollback counter : "); nova_put_u64(g_tpm_rollback_counter); nova_putln("");
    nova_puts("  TPM trust epoch ctr  : "); nova_put_u64(g_tpm_trust_epoch_counter); nova_putln("");
    nova_puts("  Active rollback idx  : "); nova_put_u64(g_active_rollback_index); nova_putln("");
    nova_puts("  Measured boot        : "); nova_putln(g_measured_boot?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Event-log evidence   : "); nova_putln(g_event_log_verified?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Release policy       : "); nova_putln(g_release_policy_verified?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Release trust epoch  : "); nova_put_u64(g_release_trust_epoch); nova_putln("");
    nova_puts("  Release key ID       : "); nova_putln(g_release_active_key_id);
    nova_puts("  Services loaded      : "); nova_put_u64((unsigned long)g_service_count); nova_putln("");
    nova_puts("  Trust                : "); nova_putln(trust_name());
    nova_puts("  Integrity            : "); nova_putln(integrity_name());
    nova_puts("  System dm-verity     : "); nova_putln(verity_name());
    nova_puts("  Recovery dm-verity   : "); nova_putln(recovery_verity_name());
    nova_puts("  Persistent storage   : "); nova_putln(g_storage_ready?"READY":"NOT READY");
    nova_puts("  Active slot          : "); char as[2]={g_active_slot,'\0'}; nova_putln(as);
    nova_puts("  Boot state           : "); nova_putln(g_boot_degraded?"DEGRADED":"OK");
    nova_putln("  Runtime state        : /run/nova/boot.state");
    nova_putln("  Runtime log          : /run/log/novad.log");
}

static void cmd_layout(void) {
    nova_putln("NovaOS Stage-4D storage policy");
    nova_puts("  /System    : "); nova_putln(g_system_readonly?"READ-ONLY":"NOT SEALED");
    nova_puts("  /Data      : "); nova_putln(g_data_writable?"WRITABLE":"NOT WRITABLE");
    nova_puts("  /Recovery  : "); nova_putln(g_recovery_readonly?"READ-ONLY":"NOT SEALED");
    nova_puts("  GPT storage: "); nova_putln(g_storage_persistent?"ATTACHED":"FALLBACK");
    nova_puts("  Active slot: "); char a[2]={g_active_slot,'\0'}; nova_putln(a);
    nova_putln("  Partitions : ESP + System A + System B + Recovery + Data");
}

static void cmd_trust(void) {
    nova_putln("NovaOS Stage-4D trust foundation");
    nova_puts("  State       : "); nova_putln(trust_name());
    nova_puts("  Profile     : "); nova_putln(NOVA_TRUST_PROFILE);
    nova_puts("  Key ID      : "); nova_putln(NOVA_TRUST_KEY_ID);
    nova_putln("  Algorithm   : RSA-2048 / PKCS#1 v1.5 / SHA-256");
    nova_puts("  Root cert   : "); nova_putln(ROOT_CERT_PATH);
    nova_puts("  Root pin    : "); nova_putln(NOVA_ROOT_CERT_SHA256);
    nova_puts("  Manifest    : "); nova_putln(INTEGRITY_MANIFEST);
    nova_puts("  Signature   : "); nova_putln(INTEGRITY_SIGNATURE);
    nova_putln("  Kernel verify: Linux asymmetric-key/keyctl");
    nova_puts("  Release policy: "); nova_putln(g_release_policy_verified?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Policy format : "); nova_put_u64((unsigned long)g_release_policy_format); nova_putln("");
    nova_puts("  Policy state  : "); nova_putln(g_release_policy_state);
    nova_puts("  Policy SHA-256: "); nova_putln(g_release_policy_sha256[0]?g_release_policy_sha256:"unavailable");
    nova_puts("  Release epoch : "); nova_put_u64(g_release_trust_epoch); nova_putln("");
    nova_puts("  Epoch state   : "); nova_putln(release_epoch_state(g_release_trust_epoch));
    nova_puts("  Preboot bind  : "); nova_putln(g_preboot_release_policy_verified?"VERIFIED":"not required/unavailable");
    nova_puts("  Preboot SHA   : "); nova_putln(g_preboot_release_policy_sha256[0]?g_preboot_release_policy_sha256:"unavailable");
    nova_puts("  Release key   : "); nova_putln(g_release_active_key_id);
    nova_puts("  Release cert  : "); nova_putln(g_release_active_cert_sha256[0]?g_release_active_cert_sha256:"unavailable");
    nova_puts("  Next key      : "); nova_putln(g_release_next_key_id);
    nova_puts("  Next cert     : "); nova_putln(g_release_next_cert_sha256[0]?g_release_next_cert_sha256:"unavailable");
    nova_puts("  TPM trust ctr : "); nova_put_u64(g_tpm_trust_epoch_counter); nova_putln("");
    nova_puts("  Revoked keys  : "); nova_putln(g_release_revoked_key_ids);
    nova_puts("  Firmware root: "); nova_putln(firmware_trust_name());
    nova_puts("  UEFI signer  : "); nova_putln(NOVA_UEFI_CERT_SHA256);
    nova_putln("  Initramfs    : BUILT INTO SIGNED EFI-STUB KERNEL");
    nova_putln("Firmware authenticates the signed EFI-stub kernel; that signed image contains the initramfs and its pinned NovaOS content-trust root.");
}

static void cmd_firmware(void) {
    nova_putln("NovaOS Stage-4D firmware trust");
    nova_puts("  State         : "); nova_putln(firmware_trust_name());
    nova_puts("  UEFI          : "); nova_putln(g_firmware_uefi?"DETECTED":"NOT DETECTED");
    nova_puts("  Secure Boot   : "); nova_putln(g_firmware_secure_boot?"ENABLED":"DISABLED/UNAVAILABLE");
    nova_puts("  efivarfs      : "); nova_putln(g_efivarfs_active?"MOUNTED":"UNAVAILABLE");
    nova_puts("  Setup Mode    : ");
    if(g_firmware_setup_mode<0) nova_putln("UNKNOWN"); else nova_putln(g_firmware_setup_mode?"ENABLED":"DISABLED");
    nova_puts("  Signer pin    : "); nova_putln(NOVA_UEFI_CERT_SHA256);
    nova_putln("  EFI path      : EFI/BOOT/BOOTX64.EFI (x86_64 acceptance)");
    nova_putln("  Initramfs     : BUILT INTO SIGNED EFI-STUB KERNEL");
    nova_putln("  Runtime vars  : SecureBoot + SetupMode via efivarfs");
}

static void cmd_tpm(void) {
    nova_putln("NovaOS Stage-4D TPM / measured-boot state");
    nova_puts("  TPM state             : "); nova_putln(tpm_state_name());
    nova_puts("  TPM device node       : "); nova_putln(nova_tpm2_device_node_present()?"PRESENT":"MISSING");
    nova_puts("  PCR read source       : "); nova_putln(nova_tpm2_last_pcr_source());
    nova_puts("  Last TPM RC           : "); nova_put_u64(nova_tpm2_last_rc()); nova_puts(" ("); nova_puts(nova_tpm2_rc_name(nova_tpm2_last_rc())); nova_putln(")");
    nova_puts("  Counter initialized   : "); nova_putln(nova_tpm2_counter_initialized()?"yes":"no (logical generation 0)");
    nova_puts("  Raw TPM counter       : "); nova_put_u64(nova_tpm2_last_raw_counter()); nova_putln("");
    nova_puts("  Raw counter baseline  : "); nova_put_u64(nova_tpm2_last_counter_base()); nova_putln("");
    nova_puts("  Baseline written      : "); nova_putln(nova_tpm2_baseline_written()?"yes":"no");
    nova_puts("  Baseline write-locked : "); nova_putln(nova_tpm2_baseline_locked()?"yes":"no");
    nova_puts("  Measurement failure   : "); nova_putln(g_measurement_failed?"YES":"no");
    char probe_sysfs[65], probe_direct[65];
    int p7s=nova_tpm2_read_pcr_sha256_sysfs_only(7,probe_sysfs);
    nova_puts("  Live PCR7 sysfs       : "); nova_putln(p7s?probe_sysfs:"UNAVAILABLE");
    int p7d=nova_tpm2_read_pcr_sha256_direct_only(7,probe_direct);
    nova_puts("  Live PCR7 direct      : "); nova_putln(p7d?probe_direct:"UNAVAILABLE");
    int p14s=nova_tpm2_read_pcr_sha256_sysfs_only(14,probe_sysfs);
    nova_puts("  Live PCR14 sysfs      : "); nova_putln(p14s?probe_sysfs:"UNAVAILABLE");
    int p14d=nova_tpm2_read_pcr_sha256_direct_only(14,probe_direct);
    nova_puts("  Live PCR14 direct     : "); nova_putln(p14d?probe_direct:"UNAVAILABLE");
    nova_puts("  Probe TPM RC          : "); nova_put_u64(nova_tpm2_last_rc()); nova_putln("");
    nova_putln("  NV rollback index     : 0x01504e56");
    nova_putln("  NV baseline index     : 0x01504e57 (write-once baseline)");
    nova_putln("  NV trust epoch index  : 0x01504e58");
    nova_putln("  NV trust baseline     : 0x01504e59 (write-once baseline)");
    nova_puts("  TPM trust epoch ctr   : "); nova_put_u64(g_tpm_trust_epoch_counter); nova_putln("");
    nova_puts("  TPM rollback counter  : "); nova_put_u64(g_tpm_rollback_counter); nova_putln("");
    nova_puts("  Active rollback index : "); nova_put_u64(g_active_rollback_index); nova_putln("");
    nova_puts("  Counter provisioned   : "); nova_putln(g_tpm_counter_created?"this boot":"existing / not created");
    nova_puts("  Commit recovery       : "); nova_putln(g_tpm_counter_commit_recovered?"RECOVERED":"not needed");
    nova_puts("  Anti-rollback reject  : "); nova_putln(g_antirollback_rejected?"YES":"no");
    nova_puts("  Measured boot         : "); nova_putln(g_measured_boot?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Event-log replay      : "); nova_putln(g_event_log_verified?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Event-log error       : "); nova_putln(g_event_log_error);
    nova_puts("  Event-log SHA-256     : "); nova_putln(g_event_log_sha256[0]?g_event_log_sha256:"unavailable");
    nova_puts("  PCR7 replayed         : "); nova_putln(g_event_log_replayed_pcr7[0]?g_event_log_replayed_pcr7:"unavailable");
    nova_puts("  PCR 7 SHA-256         : "); nova_putln(g_pcr7_sha256[0]?g_pcr7_sha256:"unavailable");
    nova_puts("  PCR 7 source          : "); nova_putln(g_pcr7_source);
    nova_puts("  PCR 14 before         : "); nova_putln(g_pcr14_before[0]?g_pcr14_before:"unavailable");
    nova_puts("  PCR 14 before source  : "); nova_putln(g_pcr14_before_source);
    nova_puts("  Event digest          : "); nova_putln(g_measurement_digest[0]?g_measurement_digest:"unavailable");
    nova_puts("  PCR 14 after          : "); nova_putln(g_pcr14_after[0]?g_pcr14_after:"unavailable");
    nova_puts("  PCR 14 after source   : "); nova_putln(g_pcr14_after_source);
    nova_putln("  Measurement log       : /run/nova/measurements.log");
    nova_putln("  Counter mapping        : logical 0 = uninitialized counter; logical N>=1 = 1 + (raw - write-locked baseline)");
    nova_putln("  Policy                 : signed rollback_index must never trail the logical TPM generation");
}

static void cmd_evidence(void) {
    nova_putln("NovaOS Stage-4D measured-boot evidence");
    nova_puts("  Evidence status      : "); nova_putln(boot_evidence_verified()?"VERIFIED":"REJECTED");
    nova_puts("  Evidence error       : "); nova_putln(g_evidence_error);
    nova_puts("  Event log replay     : "); nova_putln(g_event_log_verified?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Event log error      : "); nova_putln(g_event_log_error);
    nova_puts("  Event log SHA-256    : "); nova_putln(g_event_log_sha256[0]?g_event_log_sha256:"unavailable");
    nova_puts("  Event log bytes      : "); nova_put_u64(g_event_log_bytes); nova_putln("");
    nova_puts("  Event2 count         : "); nova_put_u64(g_event_log_events); nova_putln("");
    nova_puts("  PCR7 event count     : "); nova_put_u64(g_event_log_pcr7_events); nova_putln("");
    nova_puts("  PCR7 live            : "); nova_putln(g_pcr7_sha256[0]?g_pcr7_sha256:"unavailable");
    nova_puts("  PCR7 replayed        : "); nova_putln(g_event_log_replayed_pcr7[0]?g_event_log_replayed_pcr7:"unavailable");
    nova_puts("  PCR14 before         : "); nova_putln(g_pcr14_before[0]?g_pcr14_before:"unavailable");
    nova_puts("  NovaOS event digest  : "); nova_putln(g_measurement_digest[0]?g_measurement_digest:"unavailable");
    nova_puts("  PCR14 after          : "); nova_putln(g_pcr14_after[0]?g_pcr14_after:"unavailable");
    nova_puts("  Slot descriptor      : "); nova_put_u64((unsigned long)g_active_slot_descriptor_format); nova_puts(" / "); nova_putln(g_active_slot_descriptor_kind);
    nova_puts("  Release policy       : "); nova_put_u64((unsigned long)g_release_policy_format); nova_puts(" / "); nova_putln(g_release_policy_state);
    nova_puts("  Policy SHA-256       : "); nova_putln(g_release_policy_sha256[0]?g_release_policy_sha256:"unavailable");
    nova_puts("  Preboot policy bind  : "); nova_putln(g_preboot_release_policy_verified?"VERIFIED":"not required/unavailable");
    nova_puts("  Release key          : "); nova_putln(g_release_active_key_id);
    nova_puts("  Trust epoch          : "); nova_put_u64(g_release_trust_epoch); nova_putln("");
    nova_puts("  TPM trust epoch ctr  : "); nova_put_u64(g_tpm_trust_epoch_counter); nova_putln("");
    nova_putln("  Evidence file        : /run/nova/boot-evidence.env");
}

static void cmd_integrity(void) {
    nova_putln("NovaOS signed integrity foundation");
    nova_puts("  Trust       : "); nova_putln(trust_name());
    nova_puts("  State       : "); nova_putln(integrity_name());
    nova_puts("  Manifest    : "); nova_putln(INTEGRITY_MANIFEST);
    nova_puts("  Entries     : "); nova_put_u64(g_integrity_total); nova_putln("");
    nova_puts("  Verified    : "); nova_put_u64(g_integrity_verified); nova_putln("");
    nova_puts("  Failed      : "); nova_put_u64(g_integrity_failed); nova_putln("");
    nova_puts("  Signature   : "); nova_putln(g_trust==TRUST_VERIFIED?"VERIFIED":"NOT TRUSTED");
    nova_putln("/System is block-verified by dm-verity and then file-verified by the signed integrity manifest; /Recovery is independently block-verified by signed dm-verity metadata.");
}

static void cmd_services(void) {
    reap_services(); if(g_service_count==0){ nova_putln("No services loaded."); return; }
    nova_putln("NAME             STATE      PID     RESTARTS  POLICY");
    for(int i=0;i<g_service_count;i++){ struct nova_service *svc=&g_services[i];
        nova_puts(svc->name); nova_puts("\t"); nova_puts(svc->pid>0?"running":"stopped"); nova_puts("\t");
        nova_put_u64(svc->pid>0?(unsigned long)svc->pid:0); nova_puts("\t"); nova_put_u64(svc->restart_count); nova_puts("\t");
        nova_puts(svc->restart_always?"always":"never");
        if (svc->is_application) {
            nova_puts("\tapp="); nova_puts(svc->app_id);
            nova_puts("\tclient="); nova_put_u64((unsigned long)svc->client_id);
        }
        nova_putln(""); }
}

static void service_action(const char *args) {
    const char *action=nova_skip_spaces(args), *space=action; while(*space && *space!=' ' && *space!='\t') space++;
    if(!*space){ nova_putln("usage: service <start|stop|restart|kill> <name>"); return; }
    char action_buf[16]; nova_size_t alen=(nova_size_t)(space-action); if(alen>=sizeof(action_buf)){ nova_putln("service: invalid action"); return; }
    for(nova_size_t i=0;i<alen;i++) action_buf[i]=action[i]; action_buf[alen]='\0';
    const char *name=nova_skip_spaces(space); struct nova_service *svc=find_service(name); if(!svc){ nova_puts("service: not found: "); nova_putln(name); return; }
    reap_services();
    if(nova_streq(action_buf,"start")){ svc->desired_running=1; start_service(svc); }
    else if(nova_streq(action_buf,"stop")) stop_service(svc);
    else if(nova_streq(action_buf,"restart")){ stop_service(svc); svc->desired_running=1; start_service(svc); }
    else if(nova_streq(action_buf,"kill")){ if(svc->pid<=0){nova_putln("service: not running");return;} long pid=svc->pid; (void)nova_kill(pid,NOVA_SIGKILL); int status=0; if(nova_wait4(pid,&status,0)==pid) handle_service_exit(svc,status); }
    else nova_putln("usage: service <start|stop|restart|kill> <name>");
}

static void cmd_uname(void){ struct nova_utsname u; nova_zero(&u,sizeof(u)); if(nova_uname(&u)<0){nova_putln("uname: syscall failed");return;} nova_puts("Kernel:       ");nova_putln(u.sysname);nova_puts("Node:         ");nova_putln(u.nodename);nova_puts("Release:      ");nova_putln(u.release);nova_puts("Machine:      ");nova_putln(u.machine); }
static void cmd_mem(void){ struct nova_sysinfo s; nova_zero(&s,sizeof(s)); if(nova_sysinfo(&s)<0){nova_putln("mem: sysinfo syscall failed");return;} unsigned long unit=s.mem_unit?s.mem_unit:1; nova_puts("Total RAM:    ");nova_put_bytes(s.totalram*unit);nova_putln("");nova_puts("Free RAM:     ");nova_put_bytes(s.freeram*unit);nova_putln("");nova_puts("Processes:    ");nova_put_u64(s.procs);nova_putln("");nova_puts("Uptime:       ");nova_put_u64((unsigned long)s.uptime);nova_putln(" s"); }

static void cmd_security(void) {
    nova_putln("NovaOS Stage-4D Security");
    nova_puts("  /System read-only           : "); nova_putln(g_system_readonly?"ENFORCED":"NOT ENFORCED");
    nova_puts("  /Recovery read-only         : "); nova_putln(g_recovery_readonly?"ENFORCED":"NOT ENFORCED");
    nova_puts("  /Data writable boundary     : "); nova_putln(g_data_writable?"ENFORCED":"FAILED");
    nova_puts("  Signed manifest trust       : "); nova_putln(trust_name());
    nova_puts("  Trust profile               : "); nova_putln(NOVA_TRUST_PROFILE);
    nova_puts("  Integrity manifest          : "); nova_putln(integrity_name());
    nova_puts("  Persistent GPT storage      : "); nova_putln(g_storage_ready?"ENFORCED":"NOT READY");
    nova_puts("  Running system slot         : "); char ss[2]={g_active_slot,'\0'}; nova_putln(ss);
    nova_putln("  Slot metadata journal       : DUAL RECORD + SEQUENCE + SHA-256");
    nova_putln("  Slot metadata power-loss    : MIRROR RECOVERY / SELF-HEAL");
    nova_putln("  Slot metadata authorization : NOT YET IMPLEMENTED");
    nova_putln("  Automatic A/B rollback       : ENABLED");
    nova_putln("  Signed inactive-slot update  : ENABLED");
    nova_putln("  dm-verity kernel readiness   : ENABLED");
    nova_puts("  System dm-verity mapping     : "); nova_putln(g_verity==VERITY_ENFORCED?"ENFORCED":"NOT ENFORCED");
    nova_puts("  System verity descriptor     : "); nova_putln(g_verity_descriptor_verified?"VERIFIED":"FAILED/UNAVAILABLE");
    nova_puts("  Recovery dm-verity mapping   : "); nova_putln(g_recovery_verity==VERITY_ENFORCED?"ENFORCED":"NOT ENFORCED");
    nova_puts("  Recovery verity descriptor   : "); nova_putln(g_recovery_verity_descriptor_verified?"VERIFIED":"FAILED/UNAVAILABLE");
    nova_puts("  Recovery payload trust       : "); nova_putln(recovery_payload_verified()?"TRUSTED":"NOT TRUSTED");
    nova_puts("  Recovery mutation policy     : "); nova_putln(mutation_health_ready()?"NORMAL-BOOT AUTHORIZED":"DIAGNOSTIC / BLOCKED");
    nova_puts("  TPM 2.0 state                : "); nova_putln(tpm_state_name());
    nova_puts("  TPM anti-rollback counter    : "); nova_put_u64(g_tpm_rollback_counter); nova_putln("");
    nova_puts("  Active rollback index        : "); nova_put_u64(g_active_rollback_index); nova_putln("");
    nova_puts("  TPM measured boot            : "); nova_putln(g_measured_boot?"VERIFIED":"NOT VERIFIED");
    nova_puts("  TPM event-log replay         : "); nova_putln(g_event_log_verified?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Delegated release policy     : "); nova_putln(g_release_policy_verified?"VERIFIED":"NOT VERIFIED");
    nova_puts("  Release trust epoch          : "); nova_put_u64(g_release_trust_epoch); nova_putln("");
    nova_puts("  Active release key           : "); nova_putln(g_release_active_key_id);
    nova_putln("  Measured boot PCR            : SHA-256 PCR 14 (NovaOS evidence event), PCR 7 replay-validated");
    nova_putln("  TPM provisioning policy      : DEVELOPMENT FOUNDATION; production ownership/auth lifecycle pending");
    nova_putln("  Network services             : NONE");
    nova_putln("  Third-party userspace        : NONE");
    nova_putln("  libc                         : NONE");
    nova_putln("  Service supervision          : ENABLED");
    nova_putln("  Package execution            : NOT IMPLEMENTED");
    nova_putln("  Signed manifest              : ENABLED");
    nova_puts("  Firmware/UEFI verified boot  : "); nova_putln(g_firmware_trust==FIRMWARE_TRUST_VERIFIED?"ENFORCED":"NOT VERIFIED");
    nova_puts("  UEFI Secure Boot variable    : "); nova_putln(g_firmware_secure_boot?"ENABLED":"DISABLED/UNAVAILABLE");
    nova_putln("  Kernel + initramfs binding   : SINGLE SIGNED EFI-STUB IMAGE");
    nova_putln("  App sandbox                  : MOUNT-NS PRIVATE-ROOT DATA/TMP UID-DROP");
    nova_puts("  Boot state                   : "); nova_putln(g_boot_degraded?"DEGRADED":"OK");
    nova_putln("Status is descriptive, not a security certification.");
}

static void unmount_persistent_filesystems(void) {
    /* Keep shutdown deterministic for ext4-backed /Data.  sync() alone flushes
     * dirty data but does not provide the same clean-unmount boundary that our
     * raw-disk recovery/update tests require.  Verified read-only trees are
     * detached first, then the writable Data filesystem last. */
    nova_sync();
    if(g_user_workspace_ready){
        long rc=-16;for(unsigned attempt=0;attempt<40;attempt++){
            rc=nova_umount2("/run/nova/user-session/home",0);if(rc>=0)break;
            if(rc!=-16)break;reap_services();struct nova_timespec pause={0,50000000};(void)nova_nanosleep(&pause,0);
        }
        if(rc>=0){g_user_workspace_ready=0;log_simple("INFO","shutdown","shared_home_unmounted","user filesystem cleanly unmounted before Data");}
        else log_simple("WARN","shutdown","shared_home_unmount_failed","user filesystem could not be cleanly unmounted; Data unmount will still be attempted");
    }
    if(g_recovery_verity_mapping_active && nova_umount2("/Recovery",0)<0)
        log_simple("WARN","shutdown","recovery_unmount_failed","Recovery could not be cleanly unmounted before shutdown");
    if(g_verity_mapping_active && nova_umount2("/System",0)<0)
        log_simple("WARN","shutdown","system_unmount_failed","System could not be cleanly unmounted before shutdown");
    if(g_storage_persistent && nova_umount2("/Data",0)<0)
        log_simple("WARN","shutdown","data_unmount_failed","Data could not be cleanly unmounted before shutdown");
    nova_sync();
}

static void ordered_shutdown(int reboot_command) {
    if(nova_getpid()!=1){nova_putln("shutdown: refused outside PID 1 test environment");return;}
    shutdown_services();
    write_boot_state();
    unmount_persistent_filesystems();
    nova_putln(reboot_command==NOVA_RB_AUTOBOOT?"Rebooting...":"Powering off...");
    nova_reboot(reboot_command);
}

#include "r24_diagnostics.inc"

static int execute_command(char *line) {
    const char *cmd=nova_skip_spaces(line); if(!*cmd) return 1; reap_services();
    if(nova_streq(cmd,"diagnostics")){cmd_r24_diagnostics();return 1;}
    if(nova_streq(cmd,"help")){cmd_help();return 1;} if(nova_streq(cmd,"version")){cmd_version();return 1;}
    if(nova_streq(cmd,"boot")){cmd_boot();return 1;} if(nova_streq(cmd,"firmware")){cmd_firmware();return 1;} if(nova_streq(cmd,"tpm")){cmd_tpm();return 1;} if(nova_streq(cmd,"evidence")){cmd_evidence();return 1;} if(nova_streq(cmd,"state")){stream_file("/run/nova/boot.state");return 1;}
    if(nova_streq(cmd,"layout")){cmd_layout();return 1;} if(nova_streq(cmd,"storage")){cmd_storage();return 1;}
    if(nova_streq(cmd,"recovery")){cmd_recovery();return 1;}
    if(nova_streq(cmd,"slot")){cmd_slot();return 1;} if(nova_startswith(cmd,"slot ")){slot_action(cmd+5);return 1;}
    if(nova_startswith(cmd,"persist ")){persist_action(cmd+8);return 1;}
    if(nova_streq(cmd,"verity")){cmd_verity();return 1;}
    if(nova_streq(cmd,"update")){cmd_update_status();return 1;} if(nova_startswith(cmd,"update ")){update_action(cmd+7);return 1;}
    if(nova_streq(cmd,"trust")){cmd_trust();return 1;} if(nova_streq(cmd,"integrity")){cmd_integrity();return 1;}
    if(nova_streq(cmd,"selftest")){nova_puts("SHA-256 self-test: ");nova_putln(nova_sha256_selftest()?"PASS":"FAIL");return 1;}
    if(nova_streq(cmd,"services")){cmd_services();return 1;} if(nova_startswith(cmd,"service ")){service_action(cmd+8);return 1;}
    if(nova_streq(cmd,"logs")){stream_file("/run/log/novad.log");return 1;} if(nova_streq(cmd,"uname")){cmd_uname();return 1;}
    if(nova_streq(cmd,"mem")){cmd_mem();return 1;} if(nova_streq(cmd,"security")){cmd_security();return 1;}
    if(nova_streq(cmd,"mounts")){stream_file("/proc/mounts");return 1;}
    if(nova_streq(cmd,"pwd")){char cwd[256]; if(nova_getcwd(cwd,sizeof(cwd))>=0)nova_putln(cwd);else nova_putln("pwd: getcwd failed");return 1;}
    if(nova_streq(cmd,"clear")){nova_puts("\033[2J\033[H");return 1;} if(nova_startswith(cmd,"echo ")){nova_putln(nova_skip_spaces(cmd+5));return 1;}
    if(nova_startswith(cmd,"cat ")){const char *path=nova_skip_spaces(cmd+4);if(!*path)nova_putln("usage: cat <path>");else stream_file(path);return 1;}
    if(nova_streq(cmd,"reboot")){ordered_shutdown(NOVA_RB_AUTOBOOT);return 1;} if(nova_streq(cmd,"poweroff")){ordered_shutdown(NOVA_RB_POWER_OFF);return 1;}
    if(nova_streq(cmd,"exit")){if(nova_getpid()==1){nova_putln("exit: PID 1 may not exit; use reboot or poweroff");return 1;}return 0;}
    nova_puts("nova: unknown command: ");nova_putln(cmd);nova_putln("Type 'help' for available commands.");return 1;
}

int nova_main(void) {
    const char hostname[]="nova";
    bootstrap_filesystems(); (void)nova_sethostname(hostname,sizeof(hostname)-1);
    log_simple("INFO","boot","filesystem_ready","filesystem bootstrap complete");
    detect_firmware_trust();
    mount_stage4_storage();
    /* Recovery is an independently authenticated diagnostic plane. Seal it
     * whenever its signed dm-verity mapping mounted successfully, even if the
     * normal System/TPM chain is not healthy enough to make g_storage_ready. */
    if(nova_getpid()==1 && g_recovery_verity==VERITY_ENFORCED && g_recovery_verity_mapping_active){
        g_recovery_readonly=seal_readonly_tree("/Recovery");
        if(!g_recovery_readonly){
            g_boot_degraded=1;
            log_simple("ERROR","recovery","seal_failed","verified Recovery mapping could not be sealed read-only; trusted diagnostic Recovery is unavailable");
        } else {
            log_simple("INFO","recovery","sealed","verified Recovery independently sealed read-only for normal or diagnostic use");
        }
    }
    if(g_storage_ready){
        load_services();
        verify_manifest_trust();
        verify_integrity_manifest();
        if(g_trust==TRUST_VERIFIED && g_integrity==INTEGRITY_VERIFIED){
            if(!load_release_policy()){g_boot_degraded=1;log_simple("ERROR","trust","release_policy_failed","delegated release signing policy could not be authenticated");}
        }
        if((g_trust!=TRUST_VERIFIED || g_integrity!=INTEGRITY_VERIFIED || !g_release_policy_verified) && !g_slot_confirmed){
            schedule_verity_rollback("pending verified slot failed trust/integrity checks; rollback scheduled");
        }
        if(!recover_tpm_counter_commit_if_needed()){g_boot_degraded=1;}
        if(!perform_measured_boot()){g_boot_degraded=1;}
        if(nova_getpid()==1){
            g_system_readonly=seal_readonly_tree("/System");
            if(!g_system_readonly || !g_recovery_readonly){g_boot_degraded=1;log_simple("ERROR","storage","seal_failed","read-only System/Recovery boundary could not be enforced");}
            else log_simple("INFO","storage","sealed","System and independently verified Recovery are sealed read-only");
        }
        prepare_test_network();
        prepare_user_workspace();
        start_enabled_services();
        write_app_lifecycle_state();
    } else {
        g_trust=TRUST_UNAVAILABLE; g_integrity=INTEGRITY_UNAVAILABLE;
        log_simple("WARN","boot","verified_system_unavailable","verified System is unavailable; services remain blocked and recovery shell stays available");
    }
    if(normal_boot_policy_satisfied()){
        g_boot_stage="READY"; log_simple("INFO","boot","ready","novad reached READY state");
    } else {
        g_boot_stage="RECOVERY_REQUIRED"; g_boot_degraded=1;
        log_simple("WARN","boot","recovery_required","verified boot policy was not satisfied; normal services remain blocked");
    }
    write_boot_evidence();
    write_boot_state();
    int app_listener=setup_app_lifecycle_listener();
    int catalog_listener=setup_app_catalog_listener();
    g_app_security_listener=setup_app_security_listener();
    g_resource_security_listener=setup_resource_security_listener();
    if((app_listener<0 || catalog_listener<0 || g_app_security_listener<0 || g_resource_security_listener<0) && nova_getpid()==1){ g_boot_degraded=1; write_boot_state(); }
    write_app_catalog_state();
    write_app_security_state();
    banner(); nova_putln("Type 'help' to inspect the Stage-4D system.");
    run_novad_event_loop(app_listener,catalog_listener);
    if(app_listener>=0){ nova_close(app_listener); (void)nova_unlink(NOVA_APP_SOCKET_PATH); }
    if(catalog_listener>=0){ nova_close(catalog_listener); (void)nova_unlink(NOVA_APP_CATALOG_SOCKET_PATH); }
    if(g_app_security_listener>=0){ nova_close(g_app_security_listener); g_app_security_listener=-1; (void)nova_unlink(NOVA_APP_SECURITY_SOCKET_PATH); }
    if(g_resource_security_listener>=0){ nova_close(g_resource_security_listener); g_resource_security_listener=-1; (void)nova_unlink(NOVA_RESOURCE_BROKER_AUTHORITY_SOCKET_PATH); }
    return 0;
}
