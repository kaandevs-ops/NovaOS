#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
for n in \
 'NOVA_EVIDENCE_FILE "/run/nova/boot-evidence.env"' \
 'event_log_verified=' 'event_log_error=' 'event_log_sha256=' 'event_log_replayed_pcr7=' \
 'event_log_bytes=' 'event_log_events=' 'event_log_pcr7_events=' \
 'release_policy_verified=' 'release_trust_epoch=' 'release_active_key_id=' 'release_next_key_id=' 'release_next_cert_sha256=' 'tpm_trust_epoch_counter=' \
 'schema=3' 'evidence_kind=novaos-boot-evidence' 'evidence_status=' 'evidence_error=' 'boot_stage=' 'normal_boot_authorized=' \
 'release_policy_format=' 'release_policy_state=' 'release_policy_sha256=' 'preboot_release_policy_verified=' 'preboot_release_policy_sha256=' 'slot_descriptor_format=' 'slot_descriptor_kind=' \
 'event_log_replay=' 'pcr7_source=' 'pcr14_before_source=' 'pcr14_after_source=' 'boot_evidence_verified' 'normal_boot_policy_satisfied' \
 'evidence_file=/run/nova/boot-evidence.env'; do grep -Fq "$n" "$SRC"; done
grep -Fq 'if(nova_streq(cmd,"evidence"))' "$SRC"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text(); b=s[s.index('static int perform_measured_boot'):s.index('static void copy_service')]
assert b.index('nova_tcg2_collect_evidence') < b.index('nova_tpm2_pcr_extend_sha256(14,digest)')
assert b.index('nova_streq(g_event_log_replayed_pcr7,g_pcr7_sha256)') < b.index('g_event_log_verified=1')
for v in ('g_event_log_sha256','g_release_active_key_id','g_release_trust_epoch'):
    assert v in b
print('Stage-4D machine-readable measured-boot evidence source contract passed.')
PY

python3 - "$ROOT/src/init/main.c" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
w=s[s.index('static void write_boot_evidence'):s.index('static int service_execution_allowed')]
assert 'nova_streq(g_evidence_error,"not-run") || nova_streq(g_evidence_error,"none")' in w
assert w.index('g_slot_journal_write_failed') < w.index('!g_storage_ready')
print('REJECTED evidence may not retain evidence_error=none after an otherwise successful measured-boot path.')
PY
