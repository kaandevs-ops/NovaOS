#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
H="$ROOT/scripts/windows/stage5a-phase15-check.py"
F="$ROOT/scripts/windows/final-stage5a-phase15-check.sh"
for marker in \
  'ASSET_STATE_PATH' \
  'trusted novad application catalog at boot' \
  "desktop_task1_icon_ready')=='yes'" \
  "desktop_task1_icon_asset_id')=='nova.icon.demo.a'" \
  "desktop_task2_icon_asset_id')=='nova.icon.demo.b'" \
  'App-A verified launcher icon mask' \
  'App-B verified launcher icon mask' \
  "state_int(x,'app_asset_resolved')>=2" \
  "state_int(x,'app_launch_resident_fallbacks')==0" \
  "x.get('stage')=='STOPPED'" \
  'shell enumerates rich metadata and verifies immutable icon assets by SHA-256' \
  'compositor atomically commits and renders trusted 24x24 A8 task icons' \
  'def main():' \
  '_text,shots=run()' \
  'report_success(shots)' \
  "if __name__=='__main__':" \
  'SCREEN_XCLIP' \
  'nova5a15-tpm-' \
  'nova5a15-mon-' \
  'sel.close()'; do
  grep -Fq "$marker" "$H" || { echo "missing Phase-15 runtime acceptance marker: $marker" >&2; exit 1; }
done
for marker in \
  'python3 "$ROOT/scripts/retain-build-files.py" "$ROOT/build"' \
  './tests/run-stage5a-phase15-package-suite.sh' \
  './tests/stage5a-phase15-acceptance-source.sh' \
  'Fresh x86_64 Stage-5A Phase-15 build' \
  'rootfs/System/Core/nova-assets' \
  'icon_asset_id=nova.icon.demo.a' \
  'icon_asset_id=nova.icon.demo.b' \
  'stage5a-phase15-check.py'; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-15 final runtime wrapper marker: $marker" >&2; exit 1; }
done
for fragile in \
  "[nova-compositor] render_loop=ACTIVE" \
  "nova-shell lifecycle connection" \
  "nova-shell catalog connection" \
  "nova-shell dynamic task-model sync" \
  "nova-assets listener" \
  "nova-shell verified icon resolution" \
  "process_lifecycle=GRACEFUL_EXIT" \
  "def require(" \
  "SCREEN_CLIP" \
  "nova5a14-"; do
  if grep -Fq "$fragile" "$H"; then
    echo "Phase-15 acceptance still contains fragile/stale harness construct: $fragile" >&2
    exit 1
  fi
done
for marker in \
  "app_value(x,'nova.demo.a','state')==APP_RUNNING" \
  "app_value(x,'nova.demo.a','generation')==1" \
  "state_int(x,'app_launch_spawned')>spawned_before" \
  "app_value(x,'nova.demo.b','state')==APP_RUNNING" \
  "application PID changed across shell-only crash/restart"; do
  grep -Fq "$marker" "$H" || { echo "missing machine-state/PID-generation runtime evidence: $marker" >&2; exit 1; }
done
python3 "$ROOT/tests/stage5a-phase15-harness-static-unit.py"
python3 -c 'import ast,sys; ast.parse(open(sys.argv[1],encoding="utf-8").read())' "$H"
bash -n "$F"
echo 'Stage-5A Phase-15 OVMF/TPM trusted-asset/rich-launcher/shell-reconstruction acceptance source contract: PASS'
