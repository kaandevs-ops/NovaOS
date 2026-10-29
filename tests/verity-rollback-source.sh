#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"
for needle in \
  'schedule_verity_rollback' \
  'active slot verity descriptor was not authenticated; rollback scheduled' \
  'dm-verity mapping creation failed for pending slot; rollback scheduled' \
  'dm-verity rejected System blocks while mounting pending slot; rollback scheduled' \
  'verity_rollback_scheduled=' \
  'RECOVERY_REQUIRED'; do grep -Fq "$needle" "$SRC"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text(); b=s[s.index('static void schedule_verity_rollback'):s.index('static void mount_stage4_storage')]
assert 'if(g_slot_confirmed || g_previous_slot==g_active_slot) return;' in b
assert 'commit_slot_metadata(fallback,failed' in b
assert 'g_next_slot=fallback' in b
print('Stage-4A verity rollback scheduling source regression passed.')
PY
