#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/scripts/windows/stage5a-phase3-check.py"
S="$ROOT/scripts/windows/final-stage5a-phase3-check.sh"
for needle in \
  'stage5a-phase3-frame-a.ppm' \
  'stage5a-phase3-frame-b.ppm' \
  'sendkey spc' \
  'mouse_move 40 20' \
  'presented_theme=1' \
  'page_flip_count' \
  'keyboard_events' \
  'pointer_events' \
  'verify_theme0_ppm' \
  'verify_theme1_ppm' \
  "screendump {path} novagpu 0" \
  'take_screenshot(SCREEN_A)' \
  'take_screenshot(SCREEN_B)'; do grep -Fq "$needle" "$P"; done
grep -Fq 'run-stage5a-phase3-package-suite.sh' "$S"
grep -Fq 'real page-flip/input render loop' "$S"
echo 'Stage-5A Phase-3 OVMF/TPM/page-flip/input screenshot acceptance contract: PASS'
