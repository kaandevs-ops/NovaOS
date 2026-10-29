#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; A="$ROOT/src/services/demo_app.c"; I="$ROOT/src/runtime/nova_input.h"
for marker in '#include "nova_font.h"' 'Interaction' 'Counter: 0' 'nova_ui_text' 'Increment' 'button_y_for_height' 'text_redraws=' 'button_activations=' 'layout_generation=' 'counter=' 'handle_widget_pointer' 'counter-increment-text-redraw' 'NOVA_KEY_ENTER'; do
  grep -Fq "$marker" "$A" "$I" >/dev/null || { echo "missing Phase-8 demo-app text/UI marker: $marker" >&2; exit 1; }
done
for marker in 'nova_ui_text' 'NOVA_SURFACE_OP_DAMAGE' 'NOVA_SURFACE_OP_COMMIT' 'resize_buffer_replacements++; g_app.layout_generation++' 'unsigned int surface_id;' 'if (buf->surface_id==SURFACE_A)' 'init_buffer(&fresh,new_id,ev->surface_id'; do grep -Fq "$marker" "$A" || { echo "missing Phase-8 redraw/layout marker: $marker" >&2; exit 1; }; done
if grep -Fq 'buf->id==g_app.buffer_a_id' "$A"; then echo 'Phase-8 resize rendering must not infer UI role from rotating buffer id' >&2; exit 1; fi
echo 'Stage-5A Phase-8 client-owned text/button/counter/resize-layout/source-role contract: PASS'
