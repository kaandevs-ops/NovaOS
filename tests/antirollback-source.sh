#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"; D="$ROOT/scripts/build-disk.sh"; U="$ROOT/scripts/build-update-bundle.sh"
for needle in \
  'rollback_index_present' \
  'desc->rollback_index<g_tpm_rollback_counter' \
  'checked_add_ul(g_tpm_rollback_counter,1UL,&pending_rollback)' \
  'desc.verity.rollback_index!=expected_rollback' \
  'target_desc.rollback_index<g_tpm_rollback_counter' \
  'recover_tpm_counter_commit_if_needed' \
  'TPM rollback/trust anti-rollback state committed'; do grep -Fq "$needle" "$SRC"; done
for needle in 'format=3' 'rollback_index=$rollback_index' 'system.slot 0' 'base-stage4d-a'; do grep -Fq "$needle" "$D"; done
for needle in 'NOVA_ROLLBACK_INDEX:-1' 'format=5' 'rollback_index=$ROLLBACK_INDEX'; do grep -Fq "$needle" "$U"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text(); b=s[s.index('if(nova_streq(args,"confirm"))'):s.index('if(nova_startswith(args,"set "))')]
# Durable journal confirmation must precede irreversible TPM counter increment.
assert b.index('commit_slot_metadata(g_active_slot') < b.index('nova_tpm2_increment_rollback_counter')
r=s[s.index('static int recover_tpm_counter_commit_if_needed'):s.index('static int perform_measured_boot')]
assert 'g_slot_confirmed' in r and 'checked_add_ul(g_tpm_rollback_counter,1UL,&pending_rollback)' in r and 'g_active_rollback_index>pending_rollback' in r and 'nova_tpm2_increment_rollback_counter' in r
print('Stage-4C anti-rollback and power-loss commit ordering passed.')
PY
