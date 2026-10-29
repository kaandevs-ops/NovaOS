#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
H="$ROOT/scripts/windows/stage5a-phase16-check.py"
W="$ROOT/scripts/windows/final-stage5a-phase16-check.sh"

[[ -f "$H" && -x "$W" ]]

for marker in \
  "SECURITY_STATE_PATH='/run/nova/app-security.state'" \
  "PROBE_READ_STATE='/run/nova/security-probe-read.state'" \
  "PROBE_NO_UI_STATE='/run/nova/security-probe-no-ui.state'" \
  "PROBE_WRITE_STATE='/run/nova/security-probe-write.state'" \
  "PROBE_UNREGISTERED_STATE='/run/nova/security-probe-unregistered.state'" \
  "state_int(x,'registry_count')==5" \
  "app3_effective_mask')==9" \
  "app4_effective_mask')==8" \
  "app5_effective_mask')==17" \
  "client_id_mismatch_denials" \
  "principal_lookup_denials" \
  "ui_surface_denials" \
  "clipboard_read_denials" \
  "clipboard_write_denials" \
  "service kill phase16-security-probe-read" \
  "service kill compositor" \
  "Stage-5A Phase-16 OVMF Secure Boot + TPM + authenticated capability-policy adversarial runtime acceptance: PASS"; do
  grep -Fq "$marker" "$H" || { echo "missing Phase-16 runtime harness marker: $marker" >&2; exit 1; }
done

for marker in \
  'python3 "$ROOT/scripts/retain-build-files.py" "$ROOT/build"' \
  './tests/run-stage5a-phase16-package-suite.sh' \
  './tests/stage5a-phase16-acceptance-source.sh' \
  'NOVA_PHASE16_ACCEPTANCE_PROBES=1 NOVA_VERSION=0.0.12-dev ./scripts/build-all.sh x86_64' \
  'python3 ./scripts/windows/stage5a-phase16-check.py'; do
  grep -Fq "$marker" "$W" || { echo "missing Phase-16 final-wrapper marker: $marker" >&2; exit 1; }
done

python3 - "$H" <<'PY'
import pathlib,sys
p=pathlib.Path(sys.argv[1])
compile(p.read_text(encoding='utf-8'),str(p),'exec')
PY
bash -n "$W"
"$ROOT/tests/stage5a-phase16-harness-static-unit.py"

echo 'Stage-5A Phase-16 OVMF/TPM adversarial principal/capability acceptance source contract: PASS'
