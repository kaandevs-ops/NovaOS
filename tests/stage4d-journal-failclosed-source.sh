#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
for needle in \
  'g_slot_journal_write_failed' \
  'slot-journal-write-failed' \
  'automatic_rollback_commit_failed' \
  'boot_attempt_commit_failed' \
  'slot_journal_write_failed=' \
  'Journal writes: '; do
  grep -Fq "$needle" "$SRC"
done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
load=s[s.index('static void load_slot_metadata'):s.index('struct nova_verity_descriptor')]
assert load.index('commit_slot_metadata(fallback,failed,next_generation') < load.index('g_active_slot=fallback')
assert 'if(!commit_slot_metadata(g_active_slot,g_previous_slot,g_slot_generation,0,next_attempts' in load
assert 'g_boot_attempts_left=next_attempts' in load
normal=s[s.index('static int normal_boot_policy_satisfied'):s.index('static int boot_evidence_verified')]
assert '!g_slot_journal_write_failed' in normal
reason=s[s.index('static void write_boot_evidence'):s.index('static int service_execution_allowed')]
assert 'slot-journal-write-failed' in reason
print('Stage-4D boot-critical journal write failures are fail-closed for normal authorization.')
PY
