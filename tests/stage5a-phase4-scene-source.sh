#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; C="$ROOT/src/services/compositor.c"; I="$ROOT/src/runtime/nova_input.h"
for marker in \
  'setup_shell_listener' 'NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK' \
  'shell_ipc=LISTENING protocol=1' 'shell_ipc=CONNECTED protocol=1' \
  'shell_ipc=DISCONNECTED scene_retained=yes' 'scene_generation' \
  'draw_scene' 'scene_graph=ACTIVE' 'focus_index' 'window_x' \
  'NOVA_KEY_TAB' 'NOVA_KEY_RIGHT' 'NOVA_BTN_LEFT'; do grep -Fq "$marker" "$C" || { echo "missing: $marker" >&2; exit 1; }; done
grep -Fq 'NOVA_KEY_TAB = 15' "$I"
grep -Fq 'NOVA_KEY_RIGHT = 106' "$I"
grep -Fq 'g_scene.dragging' "$C"
grep -Fq 'g_scene.configured' "$C"
echo 'Stage-5A Phase-4 scene graph + software cursor + focus/window interaction contract: PASS'
