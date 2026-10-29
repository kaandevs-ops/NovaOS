#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
for n in \
 'tpm_state=' 'tpm_device_node=' 'tpm_pcr_read_source=' 'tpm_last_rc=' \
 'tpm_last_rc_name=' 'tpm_counter_initialized=' 'tpm_raw_counter=' 'tpm_counter_base=' \
 'tpm_baseline_written=' 'tpm_baseline_locked=' 'measurement_failed=' \
 'tpm_nv_index=0x01504e56' 'tpm_nv_baseline_index=0x01504e57' 'tpm_rollback_counter=' \
 'active_rollback_index=' 'tpm_counter_commit_recovered=' 'antirollback_rejected=' \
 'measured_boot=' 'pcr7_sha256=' 'pcr7_source=' 'pcr14_before=' 'pcr14_before_source=' \
 'measurement_digest=' 'pcr14_after=' 'pcr14_after_source=' \
 'measurement_log=/run/nova/measurements.log'; do grep -Fq "$n" "$SRC"; done
for n in 'static void cmd_tpm(void)' 'Raw TPM counter' 'Raw counter baseline' 'Baseline write-locked' 'Live PCR7 sysfs' 'Live PCR7 direct' 'Live PCR14 sysfs' 'Live PCR14 direct' 'Probe TPM RC'; do grep -Fq "$n" "$SRC"; done
grep -Fq 'if(nova_streq(cmd,"tpm"))' "$SRC"
echo 'Stage-4C machine-readable TPM/measurement diagnostics coverage passed.'
