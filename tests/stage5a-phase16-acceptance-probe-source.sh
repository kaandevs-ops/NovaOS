#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"
BU="$ROOT/scripts/build-userland.sh"
BI="$ROOT/scripts/build-initramfs.sh"
FIX="$ROOT/tests/fixtures/stage5a-phase16"
PROBE="$FIX/security_probe.c"

for marker in \
  'principal_auth_denials=' \
  'principal_lookup_denials=' \
  'client_id_mismatch_denials=' \
  'ui_surface_denials=' \
  '_client_id=' \
  '_app_id='; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-16 acceptance machine-state marker: $marker" >&2; exit 1; }
done

python3 - "$C" <<'PY'
import re,sys
text=open(sys.argv[1],encoding='utf-8').read()
def body(name):
    m=re.search(r'static\s+[^\n]+\s+'+re.escape(name)+r'\s*\([^)]*\)\s*\{',text)
    assert m,name
    i=m.end(); depth=1
    while i<len(text) and depth:
        if text[i]=='{': depth+=1
        elif text[i]=='}': depth-=1
        i+=1
    assert depth==0,name
    return text[m.start():i]
lookup=body('lookup_security_principal')
auth=body('authenticate_client_connection')
assert 'NOVA_APP_CAP_UI_SURFACE' not in lookup, 'principal parsing must not hide the reason for ui.surface denial'
assert 'NOVA_APP_CAP_UI_SURFACE' in auth
assert 'ui_surface_denials' in auth
assert 'client_id_mismatch_denials' in auth
assert 'principal_lookup_denials' in auth
PY

for f in security-probe-read.service security-probe-no-ui.service security-probe-write.service security-probe-unregistered.service security_probe.c; do
  [[ -f "$FIX/$f" ]] || { echo "missing acceptance fixture: $f" >&2; exit 1; }
done

grep -qx 'capabilities=ui.surface,clipboard.read' "$FIX/security-probe-read.service"
grep -qx 'client_id=3' "$FIX/security-probe-read.service"
grep -qx 'restart=always' "$FIX/security-probe-read.service"
grep -qx 'capabilities=clipboard.read' "$FIX/security-probe-no-ui.service"
grep -qx 'client_id=4' "$FIX/security-probe-no-ui.service"
grep -qx 'capabilities=ui.surface,clipboard.write' "$FIX/security-probe-write.service"
grep -qx 'client_id=5' "$FIX/security-probe-write.service"
grep -qx 'kind=service' "$FIX/security-probe-unregistered.service"
! grep -q '^capabilities=' "$FIX/security-probe-unregistered.service"
! grep -q '^app_id=' "$FIX/security-probe-unregistered.service"

# Acceptance probes must never leak into ordinary production service config.
! grep -q 'phase16-security-probe' "$ROOT/configs/services/services.list"
for script in "$BU" "$BI"; do
  grep -Fq 'NOVA_PHASE16_ACCEPTANCE_PROBES' "$script"
done
for marker in \
  'nova-security-probe-read' \
  'nova-security-probe-no-ui' \
  'nova-security-probe-write' \
  'nova-security-probe-unregistered'; do
  grep -Fq "$marker" "$BU"
done
for marker in \
  'phase16-security-probe-read.service' \
  'phase16-security-probe-no-ui.service' \
  'phase16-security-probe-write.service' \
  'phase16-security-probe-unregistered.service'; do
  grep -Fq "$marker" "$BI"
done

for marker in \
  'g_probe.spoof_attempts++' \
  'send_hello(fd,99U)' \
  'NOVA_SURFACE_INPUT_TEXT_ASCII' \
  'NOVA_SURFACE_INPUT_CAPTURE_TAB' \
  'NOVA_SURFACE_INPUT_CLIPBOARD' \
  'NOVA_SURFACE_OP_SET_CLIPBOARD' \
  'NOVA_SURFACE_OP_GET_CLIPBOARD'; do
  grep -Fq "$marker" "$PROBE" || { echo "missing adversarial probe marker: $marker" >&2; exit 1; }
done

CC="${CC:-clang}"
for ARCH in x86_64 aarch64; do
  TARGET=x86_64-linux-gnu
  [[ "$ARCH" == aarch64 ]] && TARGET=aarch64-linux-gnu
  for MODE in NOVA_SECURITY_PROBE_READ_ONLY NOVA_SECURITY_PROBE_NO_UI NOVA_SECURITY_PROBE_WRITE_ONLY NOVA_SECURITY_PROBE_UNREGISTERED; do
    "$CC" --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only \
      -I"$ROOT/src/runtime" -D"$MODE"=1 "$PROBE"
  done
done

bash -n "$BU" "$BI"
echo 'Stage-5A Phase-16 acceptance-only signed adversarial-principal probe/build wiring source contract: PASS'
