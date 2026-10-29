#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; A="$ROOT/src/services/demo_app.c"; T="$ROOT/src/runtime/nova_textedit.c"; H="$ROOT/src/runtime/nova_textedit.h"
for marker in \
  'UI_SELECTION_BG' 'nova_textedit_selection_bounds' 'nova_textedit_has_selection' 'nova_textedit_select_all' \
  'nova_textedit_move_left_select' 'nova_textedit_move_right_select' 'nova_textedit_copy_selection' \
  'nova_textedit_insert_ascii_bytes' 'NOVA_INPUT_MOD_CTRL' 'NOVA_INPUT_MOD_SHIFT' \
  'NOVA_SURFACE_OP_SET_CLIPBOARD' 'NOVA_SURFACE_OP_GET_CLIPBOARD' 'NOVA_SURFACE_EVENT_CLIPBOARD' \
  'shortcut=CTRL+A action=select-all' 'shortcut=CTRL+C action=clipboard-copy' \
  'shortcut=CTRL+X action=clipboard-cut' 'shortcut=CTRL+V action=clipboard-request' \
  'clipboard=PASTE length=' 'clipboard_copies=' 'clipboard_cuts=' 'clipboard_pastes=' \
  'selection_start=' 'selection_end=' 'selection_length=' 'text_input_subset=ascii-utf8-selection-clipboard' \
  'NOVA_SURFACE_INPUT_CAPTURE_TAB|NOVA_SURFACE_INPUT_TEXT_ASCII|NOVA_SURFACE_INPUT_CLIPBOARD' \
  'stage=CONNECTED protocol=6 surfaces=2 buffers=2 roles=toplevel widgets=3 textedit=selection-clipboard transport=memfd-scm-rights'; do
  grep -Fq "$marker" "$A" || { echo "missing Phase-10 demo-app marker: $marker" >&2; exit 1; }
done
for marker in 'unsigned int anchor;' 'nova_textedit_delete_selection' 'nova_textedit_select_all' 'nova_textedit_copy_selection' 'nova_textedit_insert_ascii_bytes'; do
  grep -Fq "$marker" "$H" "$T" >/dev/null || { echo "missing Phase-10 textedit marker: $marker" >&2; exit 1; }
done
echo 'Stage-5A Phase-10 client selection/modifier/copy-cut-paste source contract: PASS'
