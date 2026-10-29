#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; A="$ROOT/src/services/demo_app.c"; T="$ROOT/src/runtime/nova_textedit.c"; W="$ROOT/src/runtime/nova_widget.c"
for marker in \
  '#include "nova_textedit.h"' '#include "nova_widget.h"' 'APP_WIDGET_TEXT_FIELD' 'APP_WIDGET_BUTTON' \
  'nova_widget_focus_next' 'nova_widget_hit_test' 'nova_textedit_insert_ascii' 'nova_textedit_backspace' \
  'nova_textedit_delete' 'nova_textedit_move_left' 'nova_textedit_move_right' 'text_input_subset=ascii-utf8' \
  'NOVA_SURFACE_INPUT_CAPTURE_TAB|NOVA_SURFACE_INPUT_TEXT_ASCII' 'key=TAB action=widget-focus-next' \
  'key=ACTIVATE action=focused-button-counter-redraw' 'pointer=TEXT_FIELD action=focus-caret-end' \
  'widget_focus=' 'text_value=' 'caret_index=' 'text_edits='; do
  grep -Fq "$marker" "$A" || { echo "missing Phase-9 demo-app marker: $marker" >&2; exit 1; }
done
for marker in 'NOVA_TEXTEDIT_CAPACITY' 'codepoint<0x20U' 'codepoint>0x7eU' 'nova_textedit_backspace' 'nova_textedit_delete'; do grep -Fq "$marker" "$ROOT/src/runtime/nova_textedit.h" "$T" >/dev/null || { echo "missing textedit marker: $marker" >&2; exit 1; }; done
for marker in 'NOVA_WIDGET_MAX_NODES' 'NOVA_WIDGET_FLAG_FOCUSABLE' 'nova_widget_focus_next' 'nova_widget_hit_test'; do grep -Fq "$marker" "$ROOT/src/runtime/nova_widget.h" "$W" >/dev/null || { echo "missing widget marker: $marker" >&2; exit 1; }; done
echo 'Stage-5A Phase-9 client-side widget tree + focus/text-edit/caret source contract: PASS'
