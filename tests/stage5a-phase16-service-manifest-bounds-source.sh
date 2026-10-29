#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAIN="$ROOT/src/init/main.c"
FIX="$ROOT/tests/fixtures/stage5a-phase16/security-probe-unregistered.service"
HARNESS="$ROOT/scripts/windows/stage5a-phase16-check.py"

# The registry identity is bounded and manifest parsing must reject rather than truncate.
grep -Eq '^#define SERVICE_NAME_MAX[[:space:]]+32$' "$MAIN"
grep -Eq '^#define SERVICE_PATH_MAX[[:space:]]+160$' "$MAIN"
grep -Fq 'nova_strlen(value)>=sizeof(svc->name)) return -1;' "$MAIN"
grep -Fq 'nova_strlen(value)>=sizeof(svc->exec)) return -1;' "$MAIN"

grep -qx 'name=phase16-security-probe-foreign' "$FIX"
! grep -q '^name=phase16-security-probe-unregistered$' "$FIX"
grep -Fq "service start phase16-security-probe-foreign" "$HARNESS"
! grep -Fq "service start phase16-security-probe-unregistered" "$HARNESS"
grep -Fq 'phase16-security-probe-unregistered.service' "$ROOT/scripts/windows/final-stage5a-phase16-check.sh"
! grep -Fq 'phase16-security-probe-foreign.service' "$ROOT/scripts/windows/final-stage5a-phase16-check.sh"

python3 - "$ROOT" <<'PY'
from pathlib import Path
import re, sys
root=Path(sys.argv[1])
main=(root/'src/init/main.c').read_text()
name_cap=int(re.search(r'^#define SERVICE_NAME_MAX\s+(\d+)$',main,re.M).group(1))
path_cap=int(re.search(r'^#define SERVICE_PATH_MAX\s+(\d+)$',main,re.M).group(1))
files=list((root/'configs/services').glob('*.service'))+list((root/'tests/fixtures/stage5a-phase16').glob('*.service'))
assert files
for path in files:
    fields={}
    for raw in path.read_text().splitlines():
        if '=' in raw and not raw.lstrip().startswith('#'):
            k,v=raw.split('=',1); fields[k.strip()]=v.strip()
    if 'name' in fields:
        assert 0 < len(fields['name'].encode()) < name_cap, (path, fields['name'], name_cap)
    if 'exec' in fields:
        assert 0 < len(fields['exec'].encode()) < path_cap, (path, fields['exec'], path_cap)
assert len('phase16-security-probe-unregistered'.encode()) >= name_cap
assert len('phase16-security-probe-foreign'.encode()) < name_cap
PY

echo 'Stage-5A Phase-16 strict service-manifest name/exec bounds + no-silent-truncation source contract: PASS'
