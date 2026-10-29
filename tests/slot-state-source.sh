#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$ROOT/src/init/main.c" <<'PY'
from pathlib import Path
import sys
src = Path(sys.argv[1]).read_text()
start = src.index('static void slot_action(')
end = src.index('static void persist_action(', start)
block = src[start:end]
set_marker = 'g_previous_slot=g_active_slot; g_next_slot=target; g_slot_generation=next_generation; g_slot_confirmed=0;'
confirm_marker = 'g_next_slot=g_active_slot; g_slot_confirmed=1;'
for name, marker in [('slot set', set_marker), ('slot confirm', confirm_marker)]:
    pos = block.find(marker)
    if pos < 0:
        raise SystemExit(f'{name}: mutation marker missing')
    tail = block[pos:pos+600]
    if 'write_boot_state();' not in tail:
        raise SystemExit(f'{name}: boot.state refresh missing after mutation')
print('Stage-4A slot live-state source regression passed.')
PY
