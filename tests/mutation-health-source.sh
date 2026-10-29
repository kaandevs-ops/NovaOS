#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
g=s[s.index('static int mutation_health_ready'):s.index('static void update_action')]
assert 'return normal_boot_policy_satisfied();' in g
u=s[s.index('static void update_action'):s.index('static int recover_tpm_counter_commit_if_needed')]
apply=u[u.index('if(nova_streq(args,"apply"))'):]
assert apply.index('mutation_health_ready()') < apply.index('verify_update_bundle(&desc,1)')
slot=s[s.index('static void slot_action'):s.index('static void persist_action')]
setb=slot[slot.index('if(nova_startswith(args,"set "))'):]
assert setb.index('mutation_health_ready()') < setb.index('verify_slot_verity_descriptor_ex') < setb.index('commit_slot_metadata(target')
confirm=slot[slot.index('if(nova_streq(args,"confirm"))'):slot.index('if(nova_startswith(args,"set "))')]
assert 'if(!normal_boot_policy_satisfied())' in confirm
persist=s[s.index('static void persist_action'):s.index('static void banner')]
assert persist.count('mutation_health_ready()') >= 2
assert 'persist: mutation refused outside fully authorized normal boot' in persist
print('Stage-4D persistent mutation paths share the unified normal-boot authorization gate.')
PY
