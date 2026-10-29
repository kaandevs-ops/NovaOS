#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/scripts/windows/stage5a-phase5-check.py"; F="$ROOT/scripts/windows/final-stage5a-phase5-check.sh"
for marker in \
  "b'[nova-demo-app] stage=CONNECTED protocol=2 surfaces=2'" \
  "state_int(s,'client_surface_count')==2" "state_int(s,'client_focus_surface')==100" \
  "state_int(s,'pointer_surface_id')==101" "hmp_command('sendkey tab')" "hmp_command('sendkey right')" \
  "hmp_command('mouse_move 10 10')" "service stop demo-app" "service start demo-app" "service kill demo-app" \
  'stage5a-phase5-surfaces.ppm' 'stage5a-phase5-app-focus.ppm' 'stage5a-phase5-client-move.ppm' 'stage5a-phase5-client-cleanup.ppm' \
  "expect_pixel(shots['a'],700,380,(51,204,170),'surface100 border')" \
  "expect_pixel(shots['a'],860,500,(68,51,85),'surface101 overlap top content')" \
  "expect_pixel(shots['focus'],700,380,(255,204,0),'focused client surface border')" \
  "expect_pixel(shots['move'],732,380,(255,204,0),'client surface moved by app commit')" \
  "expect_pixel(shots['clean'],740,430,(19,26,36),'client surface cleanup background')"; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-5 acceptance marker: $marker" >&2; exit 1; }
done
grep -Fq 'run-stage5a-phase5-package-suite.sh' "$F"
grep -Fq 'stage5a-phase5-check.py' "$F"
grep -Fq 'System/Apps/nova-demo-app' "$F"
echo 'Stage-5A Phase-5 OVMF/TPM/client-surface/input-routing/crash-cleanup screenshot acceptance contract: PASS'
