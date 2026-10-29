#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
for needle in \
  'NOVA_MEASUREMENTS_LOG "/run/nova/measurements.log"' \
  'nova_tpm2_read_pcr_sha256(7,g_pcr7_sha256)' \
  'nova_tpm2_read_pcr_sha256(14,g_pcr14_before)' \
  'NovaOS Stage-4D measured boot evidence v2' \
  'measurement_format=2' 'evidence_schema=3' \
  'slot_descriptor_format=' 'slot_descriptor_kind=' \
  'rollback_index=' 'tpm_rollback_counter=' \
  'system_root=' 'recovery_root=' \
  'secure_boot=' 'firmware_trust=' \
  'pcr7=' 'event_log_sha256=' 'event_log_replayed_pcr7=' \
  'release_policy_sha256=' 'preboot_release_policy_sha256=' \
  'release_active_cert_sha256=' 'release_key_id=' \
  'trust_epoch=' 'tpm_trust_epoch_counter=' 'uefi_db_cert_sha256=' \
  'nova_tpm2_pcr_extend_sha256(14,digest)' \
  'nova_streq(expected_hex,g_pcr14_after)' \
  'g_tpm_state==TPM_STATE_READY && g_measured_boot && g_event_log_verified' \
  'measured_boot='; do grep -Fq "$needle" "$SRC"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text(); b=s[s.index('static int perform_measured_boot'):s.index('static void copy_service')]
# Every field in the v2 measurement preimage must already be authenticated or
# live-verified before PCR14 is extended.
for field in (
    'NOVA_VERSION','g_active_slot','g_active_slot_descriptor_format','g_active_slot_descriptor_kind',
    'g_active_rollback_index','g_tpm_rollback_counter','g_verity_root_hash','g_recovery_verity_root_hash',
    'g_firmware_secure_boot','firmware_trust_name()','g_pcr7_sha256','g_event_log_sha256',
    'g_event_log_replayed_pcr7','g_release_policy_sha256','g_preboot_release_policy_sha256',
    'g_release_active_cert_sha256','g_release_active_key_id','g_release_trust_epoch',
    'g_tpm_trust_epoch_counter','NOVA_UEFI_CERT_SHA256'):
    assert field in b, field
assert b.index('nova_streq(g_event_log_replayed_pcr7,g_pcr7_sha256)') < b.index('NovaOS Stage-4D measured boot evidence v2')
assert b.index('g_release_policy_verified') < b.index('NovaOS Stage-4D measured boot evidence v2')
assert b.index('nova_sha256_final(&ctx,digest)') < b.index('nova_tpm2_pcr_extend_sha256(14,digest)') < b.index('g_measured_boot=1')
print('Stage-4D PCR14 v2 evidence-binding source policy passed.')
PY
