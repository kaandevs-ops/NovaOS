#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
H="$ROOT/scripts/windows/stage5a-phase17-check.py"
W="$ROOT/scripts/windows/final-stage5a-phase17-check.sh"
[[ -f "$H" && -x "$W" ]]
for marker in \
  "PROBE_A_STATE='/Data/Nova/Apps/nova.sandbox.probe.a/phase17-sandbox.state'" \
  "PROBE_B_STATE='/Data/Nova/Apps/nova.sandbox.probe.b/phase17-sandbox.state'" \
  "SANDBOX_IPC_STATE_PATH='/run/nova/sandbox-ipc.state'" \
  "app-facing compositor IPC listener" \
  "connect_attempts=" \
  "state_int(x,'registry_count')==4" \
  "app3_effective_mask')==1" \
  "app4_effective_mask')==1" \
  "st.get('NoNewPrivs')!='1'" \
  "st.get('Seccomp')!='2'" \
  "service kill phase17-sandbox-probe-a" \
  "service kill compositor" \
  "legacy applications render from Phase-17 sandbox" \
  "Stage-5A Phase-17 OVMF Secure Boot + TPM + kernel-enforced application sandbox runtime acceptance: PASS"; do
  grep -Fq "$marker" "$H" || { echo "missing Phase-17 runtime harness marker: $marker" >&2; exit 1; }
done
for marker in \
  'python3 "$ROOT/scripts/retain-build-files.py" "$ROOT/build"' \
  './tests/run-stage5a-phase17-package-suite.sh' \
  './tests/stage5a-phase17-acceptance-source.sh' \
  'NOVA_PHASE17_ACCEPTANCE_PROBES=1 NOVA_VERSION=0.0.12-dev ./scripts/build-all.sh x86_64' \
  'python3 ./scripts/windows/stage5a-phase17-check.py'; do
  grep -Fq "$marker" "$W" || { echo "missing Phase-17 final-wrapper marker: $marker" >&2; exit 1; }
done
python3 - "$H" <<'PY'
import pathlib,sys
p=pathlib.Path(sys.argv[1]); compile(p.read_text(encoding='utf-8'),str(p),'exec')
PY
bash -n "$W"
"$ROOT/tests/stage5a-phase17-harness-static-unit.py"
grep -Fq "persistent_previous_pid" "$H"
grep -Fq "wait_probe_ready(proc,buf,sel,PROBE_A_STATE,a_pid2,3" "$H"

echo 'Stage-5A Phase-17 OVMF/TPM private-filesystem/seccomp sandbox acceptance source contract: PASS'
