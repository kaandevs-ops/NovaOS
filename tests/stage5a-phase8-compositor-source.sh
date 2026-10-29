#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; C="$ROOT/src/services/compositor.c"
for marker in '#include "nova_font.h"' 'font_renderer=nova-font-5x7-v1' 'text_ui=active' '"NOVAOS"' 's->title' 'nova_font_draw_text_xrgb8888'; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-8 compositor text marker: $marker" >&2; exit 1; }
done
python3 - "$C" <<'PY_COLOR'
from pathlib import Path
import re,sys
s=Path(sys.argv[1]).read_text()
assert 'nova_ui_text' in s and 'title_luma' in s and 'title_ink,title_text' in s
assert 'static void r3_text' in s and 'nova_mix_rgb' in s
PY_COLOR
echo 'Stage-5A Phase-8 compositor AA title/proportional text and legacy fallback source contract: PASS'
