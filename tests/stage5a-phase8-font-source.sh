#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; H="$ROOT/src/runtime/nova_font.h"; C="$ROOT/src/runtime/nova_font.c"; B="$ROOT/scripts/build-userland.sh"
for f in "$H" "$C"; do [[ -f "$f" ]] || { echo "missing font source: $f" >&2; exit 1; }; done
for marker in 'NOVA_FONT_GLYPH_WIDTH 5U' 'NOVA_FONT_GLYPH_HEIGHT 7U' 'nova_font_measure_text' 'nova_font_draw_text_xrgb8888' 'NOVA_FONT_UTF8_ASCII_MAX'; do grep -Fq "$marker" "$H" || { echo "missing Phase-8 font header marker: $marker" >&2; exit 1; }; done
for marker in "case 'A'" "case '0'" 'next_glyph' 'replacement=1U' 'clip_x1' 'put_pixel'; do grep -Fq "$marker" "$C" || { echo "missing Phase-8 font implementation marker: $marker" >&2; exit 1; }; done
grep -Fq 'src/runtime/nova_font.c' "$B"
echo 'Stage-5A Phase-8 freestanding bitmap-font/metrics/UTF8-fallback/clipping source contract: PASS'
