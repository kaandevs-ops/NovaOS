#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; C="$ROOT/src/services/compositor.c"
for marker in \
  'unsigned int input_flags;' 'NOVA_SURFACE_OP_SET_INPUT_FLAGS' 'input_flags_invalid' \
  'NOVA_SURFACE_INPUT_CAPTURE_TAB' 'surface_input_flags=' 'surface100_input_flags=' \
  'widget_input=active' 'widget_input=ACTIVE' \
  'ev->code==NOVA_KEY_TAB && focused_client && (focused_client->input_flags&NOVA_SURFACE_INPUT_CAPTURE_TAB)' \
  'route_client_event(NOVA_SURFACE_EVENT_KEY,g_client.focus_surface,ev->code,ev->value)' \
  'protocol=6 lifecycle=toplevel-configure-ack widget-input=tab-capture-modifiers'; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-9 compositor marker: $marker" >&2; exit 1; }
done
echo 'Stage-5A Phase-9 compositor client TAB-capture/input-routing compatibility contract: PASS'
