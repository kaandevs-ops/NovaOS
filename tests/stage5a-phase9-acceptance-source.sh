#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/scripts/windows/stage5a-phase9-check.py"; F="$ROOT/scripts/windows/final-stage5a-phase9-check.sh"
for marker in \
  'stage5a-phase9-text-field.ppm' 'stage5a-phase9-edited-text.ppm' 'stage5a-phase9-button-focus.ppm' \
  'stage5a-phase9-keyboard-activate.ppm' 'stage5a-phase9-resized-widgets.ppm' \
  "state_int(s,'surface100_input_flags')==7" "s.get('text_value')=='NOVA'" "state_int(s,'widget_focus')==2" \
  "sendkey h" "sendkey i" "sendkey backspace" "sendkey left" "sendkey x" "sendkey delete" "sendkey tab" "sendkey ret" \
  "s.get('text_value')=='NOVAX'" "state_int(s,'text_backspaces')>backspaces" "state_int(s,'text_deletes')>deletes" \
  "state_int(s,'widget_focus')==3" "state_int(s,'keyboard_activations')>keyboard_before" \
  "state_int(s,'text_field_y')==216" "state_int(s,'button_y')==234" \
  "expect_pixel(shots['field'],756,539,(255,255,255),'initial caret after NOVA')" \
  "expect_pixel(shots['edited'],762,539,(255,255,255),'caret after NOVAX')" \
  "expect_pixel(shots['button'],871,559,(255,255,255),'focused button marker')" \
  "state=wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'edited text scanout')" \
  "state=wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'button focus scanout')" \
  "state=wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'keyboard activation scanout')" \
  "state=wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'resized widget scanout')" \
  "expect_pixel(shots['resized'],724,596,(255,255,255),'resized focused text-field border')" \
  'service kill demo-app'; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-9 acceptance marker: $marker" >&2; exit 1; }
done

for marker in \
  'def client_commit_target_presented' \
  'def wait_demo_redraw_presented' \
  "commit_target=state_int(demo_state,'commits')" \
  "lambda s: client_commit_target_presented(s,commit_target,before_visual)" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'ASCII H redraw')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'ASCII I redraw')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'backspace redraw')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'caret-left redraw')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'insert-at-caret redraw')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'button focus scanout')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'keyboard activation scanout')" \
  "wait_demo_redraw_presented(proc,buf,sel,demo,commits_before,visual_before,'text-field refocus scanout')"; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-9 frame-sync marker: $marker" >&2; exit 1; }
done
grep -Fq 'run-stage5a-phase9-package-suite.sh' "$F"
grep -Fq 'stage5a-phase9-check.py' "$F"
echo 'Stage-5A Phase-9 OVMF/TPM/widget-focus/text-edit/keyboard-activation screenshot acceptance contract: PASS'
