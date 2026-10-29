#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/scripts/windows/stage5a-phase4-check.py"; F="$ROOT/scripts/windows/final-stage5a-phase4-check.sh"
for marker in \
  "'[nova-compositor] scene_graph=ACTIVE'" \
  "b'[nova-shell] stage=CONNECTED protocol=1'" \
  "hmp_command('sendkey tab')" "hmp_command('sendkey right')" \
  "hmp_command('mouse_move 40 20')" "service kill shell" \
  'stage5a-phase4-scene-a.ppm' 'stage5a-phase4-focus.ppm' \
  'stage5a-phase4-window-moved.ppm' 'stage5a-phase4-cursor.ppm' \
  'shell restart/reconnect' 'scene_retained=yes' \
  "expect_pixel(shots['a'],180,20,(36,49,66),'panel')" \
  "expect_pixel(shots['a'],20,20,(255,136,0),'panel accent tile')" \
  "expect_pixel(shots['a'],cx,cy-5,(255,255,255),'software cursor cross')" \
  "expect_pixel(shots['a'],cx,cy,(0,0,0),'software cursor center')" \
  "expect_pixel(shots['cursor'],xs,ys-5,(255,255,255),'moved software cursor cross')" \
  "expect_pixel(shots['cursor'],xs,ys,(0,0,0),'moved software cursor center')"; do grep -Fq "$marker" "$P" || { echo "missing acceptance marker: $marker" >&2; exit 1; }; done
grep -Fq 'run-stage5a-phase4-package-suite.sh' "$F"
grep -Fq 'stage5a-phase4-check.py' "$F"
grep -Fq 'System/Core/nova-shell' "$F"
echo 'Stage-5A Phase-4 OVMF/TPM/shell-IPC/scene/focus/restart screenshot acceptance contract: PASS'
