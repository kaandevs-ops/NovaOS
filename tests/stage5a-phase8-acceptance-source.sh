#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/scripts/windows/stage5a-phase8-check.py"; F="$ROOT/scripts/windows/final-stage5a-phase8-check.sh"
for marker in \
  'stage5a-phase8-text-ui.ppm' 'stage5a-phase8-counter.ppm' 'stage5a-phase8-resized-layout.ppm' \
  "font_renderer')=='nova-font-5x7-v1'" "state_int(s,'counter')==1" "state_int(s,'button_y')==174" \
  "expect_pixel(shots['text'],58,22,(255,255,255),'panel NOVAOS glyph stroke')" \
  "expect_pixel(shots['text'],712,389,(255,204,0),'focused title text N stroke')" \
  "expect_pixel(shots['text'],724,496,(255,255,255),'client HELLO H stroke')" \
  "expect_pixel(shots['counter'],834,520,(255,255,255),'counter 2 distinguishing stroke')" \
  "state_int(s,'button_y')==234" \
  "expect_pixel(shots['resized'],724,614,(255,204,0),'button relocated by resize-aware layout')" \
  'service kill demo-app'; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-8 acceptance marker: $marker" >&2; exit 1; }
done
grep -Fq 'run-stage5a-phase8-package-suite.sh' "$F"
grep -Fq 'stage5a-phase8-check.py' "$F"
echo 'Stage-5A Phase-8 OVMF/TPM/font/text/button/resize screenshot acceptance contract: PASS'
