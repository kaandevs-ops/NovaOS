#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="$ROOT" PYTHONDONTWRITEBYTECODE=1 python3 - <<'PY'
from pathlib import Path
import os,ast
r=Path(os.environ['ROOT'])
b=(r/'scripts/build-userland.sh').read_text()
i=(r/'scripts/build-initramfs.sh').read_text()
for name,text in [('build-userland',b),('build-initramfs',i)]:
    assert text.count('STEP7_PROBES="${NOVA_PHASE18_STEP7_PROBES:-0}"')==1,name
    assert 'Step-7 probes require Phase-18 acceptance-only mode' in text,name
assert b.count('build_native "$PROBE_SRC" "$ROOTFS/System/Apps/nova-step7-idle-stress"')==1
assert i.count('cp "$ROOT/tests/fixtures/stage5a-phase18-step7/idle-stress.service"')==1
assert i.count('phase18-step7-idle-stress.service" >> "$ROOTFS/System/Services/services.list"')==1
assert 'if [[ "$STEP7_PROBES" == 1 ]]; then\n  [[ -x "$ROOTFS/System/Apps/nova-step7-idle-stress" ]]' in i
assert 'if [[ "$STEP7_PROBES" == 1 ]]; then\n  cp "$ROOT/tests/fixtures/stage5a-phase18-step7/idle-stress.service"' in i
assert 'elif [[ "$STEP7_PROBES" == 1 ]]; then' not in i
ast.parse((r/'scripts/windows/stage5a-phase18-step7-r2-check.py').read_text())
p=(r/'tests/fixtures/stage5a-phase18-step7/idle_stress_probe.c').read_text()
for anchor in ('HEARTBEATS 6U','elapsed>=130ULL','elapsed>=60ULL','request_reply(f[0],650U,1)',
               'request_reply(f[2],810U,0)','[nova-step7-probe] idle=PASS'):
    assert anchor in p,anchor
print('Phase-18 Step-7 R2 acceptance-only build/initramfs reachable branches: PASS')
print('Phase-18 Step-7 R2 guest-probe and QEMU harness syntax/contracts: PASS')
PY
for target in x86_64-linux-gnu aarch64-linux-gnu; do
  clang --target="$target" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin \
    -fsyntax-only -I"$ROOT/src/runtime" \
    "$ROOT/tests/fixtures/stage5a-phase18-step7/idle_stress_probe.c"
done
echo 'Phase-18 Step-7 R2 guest test x86_64/ARM64 C syntax: PASS'
bash -n "$ROOT/scripts/build-userland.sh"
bash -n "$ROOT/scripts/build-initramfs.sh"
PYTHONDONTWRITEBYTECODE=1 bash "$ROOT/tests/run-stage5a-phase18-step7-r1-package-suite.sh"
echo 'PHASE-18 STEP-7 R2 FIXTURE + FROZEN REGRESSIONS: PASS (not QEMU)'
