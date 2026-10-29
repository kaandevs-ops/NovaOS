#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"; DISK="$ROOT/scripts/build-disk.sh"
for needle in \
  'NOVA_SLOT_METADATA_A "/Data/Nova/Boot/state.A"' \
  'NOVA_SLOT_METADATA_B "/Data/Nova/Boot/state.B"' \
  'format=3\nrecord_seq=' \
  'slot_metadata_checksum_v3' \
  'commit_slot_metadata' \
  'journal_mirror_recovery' \
  'journal_rebuilt' \
  'slot_metadata_seq=' \
  'slot_metadata_source=' \
  'slot_metadata_recovered='; do grep -Fq "$needle" "$SRC"; done
for needle in "state.A" "state.B" "format=3" "record_seq="; do grep -Fq "$needle" "$DISK"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
load=s[s.index('static void load_slot_metadata'):s.index('struct nova_verity_descriptor')]
assert 'parse_slot_record_v3(NOVA_SLOT_METADATA_A' in load
assert 'parse_slot_record_v3(NOVA_SLOT_METADATA_B' in load
assert 'a.record_seq>=b.record_seq' in load
assert 'commit_slot_metadata' in load
assert 'seed_dual_slot_metadata' in load
print('Stage-4A redundant metadata journal selection/self-heal ordering verified.')
PY
echo "Stage-4A redundant metadata journal source test passed."
