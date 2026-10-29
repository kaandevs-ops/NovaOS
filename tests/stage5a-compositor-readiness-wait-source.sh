#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/scripts/windows/stage5a-phase1-check.py"
python3 - "$P" <<'PY2'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
prompt=s.index("if b'nova> ' not in buf")
wait=s.index("probe_deadline=time.monotonic()+20", prompt)
ready=s.index("b'[nova-compositor] stage=READY'", wait)
input_marker=s.index("b'[nova-compositor] input_event_devices='", ready)
diag=s.index("for cmd in ('cat /run/nova/graphics.state','services','state')", input_marker)
poweroff=s.index("proc.stdin.write(b'poweroff\\n')", diag)
assert prompt < wait < ready < input_marker < diag < poweroff
assert "collect(proc,buf,sel)" in s[wait:diag]
print('Stage-5A compositor readiness wait / no-prompt-to-poweroff race contract: PASS')
PY2
