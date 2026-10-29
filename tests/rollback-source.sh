#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
for needle in \
  'attempts_left' 'automatic_rollback' \
  'pending slot exhausted boot attempts; rolled back to previous slot' \
  'boot_attempt' 'rollback_performed=' 'commit_slot_metadata' \
  'slot_metadata_checksum_v3' 'checked_add_ul(g_slot_generation,1UL,&next_generation)' \
  'next_attempts=g_boot_attempts_left-1UL'; do
  grep -Fq "$needle" "$SRC"
done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
load=s[s.index('static void load_slot_metadata'):s.index('struct nova_verity_descriptor')]
assert 'if(!g_slot_confirmed && g_boot_attempts_left==0)' in load
exhausted=load[load.index('if(!g_slot_confirmed && g_boot_attempts_left==0)'):load.index('} else if(!g_slot_confirmed && g_boot_attempts_left>0)')]
assert 'checked_add_ul(g_slot_generation,1UL,&next_generation)' in exhausted
assert 'commit_slot_metadata(fallback,failed,next_generation,1,0,"none",g_update_id)' in exhausted
assert exhausted.index('commit_slot_metadata(fallback,failed,next_generation,1,0,"none",g_update_id)') < exhausted.index('g_active_slot=fallback')
assert 'g_slot_journal_write_failed=1' in exhausted

attempt=load[load.index('else if(!g_slot_confirmed && g_boot_attempts_left>0)'):]
assert 'next_attempts=g_boot_attempts_left-1UL' in attempt
assert 'commit_slot_metadata(g_active_slot,g_previous_slot,g_slot_generation,0,next_attempts,g_pending_kind,g_update_id)' in attempt
assert attempt.index('commit_slot_metadata(g_active_slot,g_previous_slot,g_slot_generation,0,next_attempts,g_pending_kind,g_update_id)') < attempt.index('g_boot_attempts_left=next_attempts')
assert 'g_slot_journal_write_failed=1' in attempt
print('Stage-4D durable rollback/attempt journal ordering source regression passed.')
PY
