#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; F="$ROOT/scripts/windows/stage5a-phase12-check.py"
for marker in \
  "desktop_control_connected')=='yes'" "taskbar App-A activate" "taskbar App-B activate" "taskbar App-B close" \
  "App-B launcher" "desktop_launches')>launch_before" "surface200_buffer_id')>=10" \
  "task_activates')>shell_activate_before" "task_closes')>shell_close_before" "task_launches')>shell_launch_before" \
  "service kill demo-app" "App-A crash with desktop shell and App-B surviving" \
  'stage5a-phase12-taskbar.ppm' 'stage5a-phase12-app-switch.ppm' 'stage5a-phase12-app-closed.ppm' 'stage5a-phase12-app-launched.ppm' \
  'Stage-5A Phase-12 OVMF Secure Boot + TPM + desktop-shell/taskbar runtime acceptance: PASS'; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-12 acceptance marker: $marker" >&2; exit 1; }
done
# Geometry-aware taskbar probes must remain outside glyphs/cursor-sensitive centers.
for marker in \
  "expect_pixel(shots['taskbar'],225,16,(49,61,79)" \
  "expect_pixel(shots['switch'],360,12,(255,204,0)" \
  "expect_pixel(shots['closed'],335,16,(34,41,52)" \
  "expect_pixel(shots['launched'],360,12,(255,204,0)"; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-12 geometry probe: $marker" >&2; exit 1; }
done
echo 'Stage-5A Phase-12 OVMF/TPM/desktop-shell/taskbar/resident-launch screenshot acceptance contract: PASS'
