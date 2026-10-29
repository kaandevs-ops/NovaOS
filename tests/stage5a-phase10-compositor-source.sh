#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; C="$ROOT/src/services/compositor.c"
for marker in \
  'struct nova_clipboard_state' 'NOVA_SURFACE_CLIPBOARD_BYTES' 'g_input_modifiers' \
  'clipboard_payload_valid' 'client_clipboard_allowed' 'set_client_clipboard' 'get_client_clipboard' \
  'clear_client_clipboard("surface_destroy")' 'clear_client_clipboard("client_disconnect")' \
  'clipboard_surface_not_focused' 'clipboard_payload_invalid' 'clipboard=SET length=' 'clipboard=GET length=' \
  'modifier_mask_for_key' 'NOVA_KEY_LEFTSHIFT' 'NOVA_KEY_RIGHTSHIFT' 'NOVA_KEY_LEFTCTRL' 'NOVA_KEY_RIGHTCTRL' \
  'ev.modifiers=g_input_modifiers' 'NOVA_SURFACE_INPUT_CLIPBOARD' \
  'clipboard_length=' 'clipboard_sets=' 'clipboard_gets=' 'clipboard_clears=' \
  'protocol=6 lifecycle=toplevel-configure-ack widget-input=tab-capture-modifiers clipboard=bounded'; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-10 compositor marker: $marker" >&2; exit 1; }
done
echo 'Stage-5A Phase-10 compositor modifier routing + focused bounded clipboard ownership/cleanup contract: PASS'
