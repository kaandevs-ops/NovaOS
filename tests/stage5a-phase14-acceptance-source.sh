#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
F="$ROOT/scripts/windows/stage5a-phase14-check.py"
W="$ROOT/scripts/windows/final-stage5a-phase14-check.sh"

for marker in \
  "CATALOG_STATE_PATH='/run/nova/app-catalog.state'" \
  'trusted novad application catalog at boot' \
  "x.get('entry1_id')=='nova.demo.a'" \
  "x.get('entry1_display_name')=='Nova Demo A'" \
  "x.get('entry2_id')=='nova.demo.b'" \
  "x.get('entry2_display_name')=='Nova Demo B'" \
  "state_int(s,'desktop_task_model_count')==2" \
  "state_int(s,'desktop_task_model_generation')==catalog_generation" \
  "s.get('desktop_task1_display_name')=='Nova Demo A'" \
  "s.get('desktop_task2_display_name')=='Nova Demo B'" \
  "x.get('app_catalog_connected')=='yes'" \
  "state_int(x,'app_catalog_count')==2" \
  "state_int(x,'task_model_entries')==2" \
  'service kill shell' \
  'replacement shell catalog re-enumeration' \
  "app_value(apps_after_shell,'nova.demo.a','pid')!=a_pid_shell" \
  "app_value(apps_after_shell,'nova.demo.b','pid')!=b_pid_shell" \
  'stage5a-phase14-shell-reconstructed.ppm' \
  'Stage-5A Phase-14 OVMF Secure Boot + TPM + dynamic application-catalog/taskbar runtime acceptance: PASS'; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-14 runtime acceptance marker: $marker" >&2; exit 1; }
done

for marker in \
  "'catalog listener':'[novad][INFO][app_catalog] read-only application catalog IPC is listening'" \
  "'catalog enumeration':'[nova-shell] app_catalog=ENUMERATED count=2'" \
  "'dynamic task-model sync':'[nova-shell] desktop_task_model=SYNCED entries=2'" \
  "'dynamic task-model commit':'[nova-compositor] desktop_task_model=COMMITTED entries=2'" \
  "'shell restart policy':'[novad][INFO][services] restart_policy shell'" \
  "'firmware_secure_boot=yes'" \
  "'tpm_state=READY'"; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-14 runtime evidence marker: $marker" >&2; exit 1; }
done

for marker in \
  'run-stage5a-phase14-package-suite.sh' \
  'Fresh x86_64 Stage-5A Phase-14 build' \
  'display_name=Nova Demo A' \
  'display_name=Nova Demo B' \
  'stage5a-phase14-check.py' \
  'dynamic application-catalog/taskbar runtime'; do
  grep -Fq "$marker" "$W" || { echo "missing Phase-14 final-wrapper marker: $marker" >&2; exit 1; }
done

python3 - "$F" <<'PY_CHECK'
import sys
source=open(sys.argv[1],encoding='utf-8').read()
compile(source,sys.argv[1],'exec')
assert 'stage5a-phase13-check.py' not in source
assert "TRANSCRIPT=OUT/'stage5a-phase14.log'" in source
PY_CHECK
bash -n "$W"
echo 'Stage-5A Phase-14 OVMF/TPM catalog/task-model/shell-reconstruction acceptance source contract: PASS'
