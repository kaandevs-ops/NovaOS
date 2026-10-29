#ifndef NOVA_TPM2_H
#define NOVA_TPM2_H

/* Minimal TPM 2.0 transport used by NovaOS Stage-4C.
 * This is intentionally not a general TPM stack. It only exposes the
 * operations needed by the boot anti-rollback + measured-boot policy. */

#define NOVA_TPM2_NV_ROLLBACK_INDEX      0x01504e56UL
#define NOVA_TPM2_NV_ROLLBACK_BASE_INDEX 0x01504e57UL
#define NOVA_TPM2_NV_TRUST_EPOCH_INDEX    0x01504e58UL
#define NOVA_TPM2_NV_TRUST_BASE_INDEX     0x01504e59UL

int nova_tpm2_present(void);
int nova_tpm2_prepare_rollback_counter(unsigned long *value, int *created);
int nova_tpm2_read_rollback_counter(unsigned long *value);
int nova_tpm2_increment_rollback_counter(unsigned long expected_current, unsigned long *new_value);
int nova_tpm2_prepare_trust_epoch_counter(unsigned long *value, int *created);
int nova_tpm2_read_trust_epoch_counter(unsigned long *value);
int nova_tpm2_increment_trust_epoch_counter(unsigned long expected_current, unsigned long *new_value);
int nova_tpm2_pcr_extend_sha256(unsigned int pcr, const unsigned char digest[32]);
int nova_tpm2_read_pcr_sha256(unsigned int pcr, char hex_out[65]);
int nova_tpm2_read_pcr_sha256_sysfs_only(unsigned int pcr, char hex_out[65]);
int nova_tpm2_read_pcr_sha256_direct_only(unsigned int pcr, char hex_out[65]);
unsigned long nova_tpm2_last_rc(void);
long nova_tpm2_last_io_status(void);
const char *nova_tpm2_last_pcr_source(void);
const char *nova_tpm2_rc_name(unsigned long rc);
unsigned long nova_tpm2_last_raw_counter(void);
unsigned long nova_tpm2_last_counter_base(void);
int nova_tpm2_counter_initialized(void);
int nova_tpm2_baseline_written(void);
int nova_tpm2_baseline_locked(void);
int nova_tpm2_device_node_present(void);

#endif
