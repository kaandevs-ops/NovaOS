#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
for needle in 'normal_boot_policy_satisfied()' 'slot: confirm refused; unified normal-boot authorization policy is not satisfied' 'release_epoch_bounds(&committed_epoch,&pending_epoch)' 'checked_add_ul(g_tpm_rollback_counter,1UL,&pending_rollback)'; do grep -Fq "$needle" "$SRC"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text(); b=s[s.index('static void slot_action'):s.index('static void persist_action')]
gate=b.index('slot: confirm refused; unified normal-boot authorization policy is not satisfied')
commit=b.index('commit_slot_metadata(g_active_slot')
assert gate < commit
print('Stage-4D confirmation uses unified boot authorization plus overflow-safe TPM bounds.')
PY
