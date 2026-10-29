#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/runtime/nova_tpm2.c"; H="$ROOT/src/runtime/nova_tpm2.h"
for needle in \
  'NOVA_TPM2_NV_ROLLBACK_INDEX      0x01504e56UL' \
  'NOVA_TPM2_NV_ROLLBACK_BASE_INDEX 0x01504e57UL' \
  'TPM2_CC_NV_DEFINE_SPACE 0x0000012aUL' \
  'TPM2_CC_NV_INCREMENT    0x00000134UL' \
  'TPM2_CC_NV_WRITE        0x00000137UL' \
  'TPM2_CC_NV_WRITE_LOCK   0x00000138UL' \
  'TPM2_CC_NV_READ         0x0000014eUL' \
  'TPM2_CC_NV_READ_PUBLIC  0x00000169UL' \
  'TPM2_RC_NV_UNINITIALIZED 0x0000014aUL' \
  'TPMA_NV_WRITEDEFINE' \
  'TPMA_NV_WRITELOCKED' \
  'TPMA_NV_WRITTEN' \
  'TPM2_CC_PCR_READ        0x0000017eUL' \
  'TPM2_CC_PCR_EXTEND      0x00000182UL' \
  'nova_open_rw("/dev/tpmrm0")' \
  'nova_open_rw("/dev/tpm0")' \
  '/sys/class/tpm/tpm0/tpm_version_major' \
  '/sys/class/tpm/tpm0/pcr-sha256/' \
  'nova_tpm2_increment_rollback_counter' \
  'nova_tpm2_pcr_extend_sha256' \
  'nova_tpm2_last_io_status' \
  'nova_tpm2_last_pcr_source'; do grep -Fq "$needle" "$C" "$H"; done
python3 - "$C" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
# TPM2 presence must not depend solely on sysfs: tpmrm0 is a strong TPM2 signal.
p=s[s.index('int nova_tpm2_present(void)'):s.index('typedef struct {')]
assert p.index('nova_open_rw("/dev/tpmrm0")') < p.index('/sys/class/tpm/tpm0/tpm_version_major')
# Baseline is provisioned before the counter so interrupted first-use is distinguishable.
e=s[s.index('static int ensure_counter_and_baseline'):s.index('int nova_tpm2_prepare_rollback_counter')]
assert e.index('baseline_public_valid') < e.index('counter_public_valid')
assert e.index('nv_define_baseline') < e.index('nv_define_counter')
# Uninitialised TPM counters are a valid NovaOS logical generation 0, not a hard failure.
p=s[s.index('int nova_tpm2_prepare_rollback_counter'):s.index('int nova_tpm2_read_rollback_counter')]
assert 'if(!counter_written)' in p
assert 'if(base_written) return 0' in p
assert 'if(value) *value=0' in p
# First raw value is captured into a write-once baseline and locked; no assumption raw==1.
for x in ('nv_write_u64(NOVA_TPM2_NV_ROLLBACK_BASE_INDEX,raw)',
          'nv_write_lock(NOVA_TPM2_NV_ROLLBACK_BASE_INDEX)',
          'unsigned long logical=1UL+(raw-base)'):
    assert x in p
assert 'raw==1' not in p and 'raw-1' not in p
# Counter increment verifies logical before/after and goes through raw NV_INCREMENT once.
b=s[s.index('int nova_tpm2_increment_rollback_counter'):s.index('int nova_tpm2_pcr_extend_sha256')]
assert b.index('nova_tpm2_prepare_rollback_counter(&before') < b.index('nv_increment_raw()') < b.index('nova_tpm2_prepare_rollback_counter(&after')
assert 'after!=before+1UL' in b
# Existing counter/baseline public areas are shape-validated.
for fn,needles in (
    ('counter_public_valid',('TPMA_NV_NT_COUNTER','TPMA_NV_OWNERREAD','TPMA_NV_OWNERWRITE','data_size!=8')),
    ('baseline_public_valid',('TPMA_NV_WRITEDEFINE','TPMA_NV_OWNERREAD','TPMA_NV_OWNERWRITE','data_size!=8')),
):
    q=s[s.index('static int '+fn):]
    q=q[:q.index('\n}\n')+3]
    for x in needles: assert x in q
# PCR direct path validates selection and digest bounds; stable sysfs path remains primary compatibility path.
d=s[s.index('static int read_pcr_sha256_direct'):s.index('int nova_tpm2_read_pcr_sha256_sysfs_only')]
for x in ('TPM2_CC_PCR_READ','TPM2_ALG_SHA256','requested_seen','digests<1UL','digests>8UL','dsz!=32U','digest_to_hex'):
    assert x in d
r=s[s.index('int nova_tpm2_read_pcr_sha256(unsigned int pcr'):]
assert 'if(read_pcr_sha256_sysfs(pcr,hex_out)) return 1;' in r
assert 'return read_pcr_sha256_direct(pcr,hex_out);' in r
print('Stage-4C TPM2 transport, NV baseline/counter mapping, presence and PCR policy passed.')
PY
