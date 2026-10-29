#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/tests/fixtures/stage5a-phase18-step6/session_stress_probe.c"
[[ -f "$SRC" && -f "$ROOT/scripts/windows/stage5a-phase18-step6-r2-check.py" ]]
python3 - "$ROOT" <<'PY'
from pathlib import Path
import sys
r=Path(sys.argv[1]);b=(r/'scripts/build-userland.sh').read_text();i=(r/'scripts/build-initramfs.sh').read_text()
for t in (b,i):
    assert 'NOVA_PHASE18_STEP6_PROBES' in t
    assert 'STEP6_PROBES' in t
    assert 'Phase-18 acceptance mode exclusively' in t
assert 'nova-step6-session-stress' in b
assert 'phase18-step6-session-stress.service' in i
src=(r/'tests/fixtures/stage5a-phase18-step6/session_stress_probe.c').read_text()
for s in ('STRESS_SOCKETS 4U','rejected_no_fd','four=PASS','fairness=PASS','fifth=PASS','reuse=PASS'):
    assert s in src,s
print('Phase-18 Step-6 R2 acceptance-only build and manifest source contract: PASS')
PY
# Exercise the actual mutually exclusive Step-7/6/5 shell branches with mocked copy.
python3 "$ROOT/tests/r16-initramfs-branch-test.py"
# Step-6 R2: a closed SEQPACKET socket with unread request can yield
# -ECONNRESET; msghdr.msg_controllen is not an output count on error.
python3 - "$ROOT/tests/fixtures/stage5a-phase18-step6/session_stress_probe.c" <<'PY_RESET'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
assert '#define STRESS_ECONNRESET 104L' in s
assert 'if(n==-STRESS_ECONNRESET)' in s
assert 'return n==0 && msg.msg_controllen==0 &&' in s
assert 'return n<=0 && msg.msg_controllen==0;' not in s
assert 'fifth_connect=FAIL' in s
print('Phase-18 Step-6 R2 AF_UNIX ECONNRESET/ancillary regression guard: PASS')
PY_RESET
PYTHONDONTWRITEBYTECODE=1 python3 - "$ROOT/scripts/windows/stage5a-phase18-step6-r2-check.py" <<'PY'
import ast,sys
from pathlib import Path
ast.parse(Path(sys.argv[1]).read_text())
print("Phase-18 Step-6 R2 QEMU harness Python syntax: PASS")
PY
for target in x86_64-linux-gnu aarch64-linux-gnu; do
  clang --target="$target" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin \
    -fsyntax-only -I"$ROOT/src/runtime" "$SRC"
done
echo 'Phase-18 Step-6 R2 broker stress probe dual-arch syntax: PASS'
PYTHONDONTWRITEBYTECODE=1 bash "$ROOT/tests/run-stage5a-phase18-step6-r1-package-suite.sh"
echo 'PHASE-18 STEP-6 R2 ACCEPTANCE FIXTURE + FROZEN REGRESSIONS: PASS (not QEMU)'
