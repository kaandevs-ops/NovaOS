#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
F="$ROOT/scripts/windows/stage5a-phase13-check.py"
W="$ROOT/scripts/windows/final-stage5a-phase13-check.sh"
for marker in \
  "APP_STATE_PATH='/run/nova/app-lifecycle.state'" \
  'configured cold desktop with no application clients' \
  "s.get('scene_configured')=='yes'" \
  "state_int(s,'scene_generation')>=1" \
  'App-A novad cold process spawn' \
  'App-B novad cold process spawn' \
  'two cold-launched independent GUI clients' \
  'cross-client clipboard paste' \
  'App-B graceful process exit and novad reap' \
  'App-B fresh PID/generation relaunch' \
  'novad independent App-A crash restart' \
  "app_launch_resident_fallbacks')==0" \
  'stage5a-phase13-cold-boot.ppm' \
  'stage5a-phase13-cold-launched.ppm' \
  'stage5a-phase13-cross-client-clipboard.ppm' \
  'stage5a-phase13-app-stopped.ppm' \
  'stage5a-phase13-app-relaunched.ppm' \
  'stage5a-phase13-crash-isolation.ppm' \
  'TRANSCRIPT.write_bytes(bytes(buf))' \
  "'client2_connected'" \
  "'client2_surface_count'" \
  "'client2_buffer_count'" \
  'Stage-5A Phase-13 OVMF Secure Boot + TPM + real application-process lifecycle runtime acceptance: PASS'; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-13 acceptance marker: $marker" >&2; exit 1; }
done
for marker in \
  'firmware_secure_boot' "x.get('tpm_state')=='READY'" \
  "app_value(x,'nova.demo.a','pid')==0" "app_value(x,'nova.demo.b','pid')==0" \
  "app_value(x,'nova.demo.b','pid')!=b_pid_1" \
  "app_value(x,'nova.demo.b','generation')==b_gen_1+1" \
  "app_value(x,'nova.demo.a','pid')!=a_pid_before" \
  "app_value(x,'nova.demo.a','generation')==a_gen_before+1"; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-13 PID/generation/security proof: $marker" >&2; exit 1; }
done
for marker in \
  'run-stage5a-phase13-package-suite.sh' \
  'Fresh x86_64 Stage-5A Phase-13 build' \
  'stage5a-phase13-check.py' \
  'real application-process lifecycle runtime'; do
  grep -Fq "$marker" "$W" || { echo "missing Phase-13 final-wrapper marker: $marker" >&2; exit 1; }
done
python3 - "$F" <<'PY_CHECK'
import sys
source=open(sys.argv[1],encoding="utf-8").read()
compile(source,sys.argv[1],"exec")
PY_CHECK
bash -n "$W"
echo 'Stage-5A Phase-13 OVMF/TPM cold-launch/graceful-stop/relaunch/crash-isolation acceptance source contract: PASS'
