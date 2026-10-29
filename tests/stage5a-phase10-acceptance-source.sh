#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/scripts/windows/stage5a-phase10-check.py"; F="$ROOT/scripts/windows/final-stage5a-phase10-check.sh"
for marker in \
  'stage5a-phase10-selection.ppm' 'stage5a-phase10-replaced.ppm' 'stage5a-phase10-cleared.ppm' \
  'stage5a-phase10-pasted.ppm' 'stage5a-phase10-cut.ppm' 'stage5a-phase10-resized.ppm' \
  "sendkey shift-right" "sendkey ctrl-a" "sendkey ctrl-c" "sendkey ctrl-x" "sendkey ctrl-v" \
  "state_int(s,'selection_start')==1" "state_int(s,'selection_end')==e" \
  "s.get('text_value')=='NXXA'" "state_int(s,'clipboard_length')==4" \
  "state_int(s,'clipboard_owner_surface')==100" "state_int(s,'clipboard_copies')>copies_before" \
  "state_int(s,'clipboard_cuts')>cuts_before" "state_int(s,'clipboard_pastes')>pastes_before" \
  "state_int(s,'clipboard_clears')>clears_before" \
  "expect_pixel(shots['selection'],739,539,(90,104,121),'OV selection highlight')" \
  "expect_pixel(shots['replaced'],739,539,(31,43,58),'selection highlight cleared after replacement')" \
  "expect_pixel(shots['pasted'],756,539,(255,255,255),'caret after pasted NXXA')" \
  "expect_pixel(shots['resized'],756,599,(255,255,255),'resized pasted text caret')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'selection redraw')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'replacement redraw')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,state_int(state_before,'client_commits'),visual_before,'paste redraw')" \
  'service kill demo-app'; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-10 acceptance marker: $marker" >&2; exit 1; }
done
grep -Fq 'run-stage5a-phase10-package-suite.sh' "$F"
grep -Fq 'stage5a-phase10-check.py' "$F"
echo 'Stage-5A Phase-10 OVMF/TPM/selection/clipboard/resize/crash-cleanup screenshot acceptance contract: PASS'
