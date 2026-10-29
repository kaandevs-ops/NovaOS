#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"
for needle in \
  'struct nova_scanout_buffer buffers[2]' \
  'double_buffer=yes' \
  'render_loop=ACTIVE' \
  'NOVA_DRM_IOCTL_MODE_PAGE_FLIP' \
  'NOVA_DRM_MODE_PAGE_FLIP_EVENT' \
  'consume_drm_events' \
  'NOVA_DRM_EVENT_FLIP_COMPLETE' \
  'nova_poll' \
  'open_input_set' \
  'nova_open_ro_nonblock' \
  'NOVA_EV_KEY' \
  'NOVA_EV_REL' \
  'NOVA_KEY_SPACE' \
  'keyboard_events' \
  'pointer_events' \
  'page_flip_count' \
  'frame_sequence' \
  'presented_theme'; do grep -Fq "$needle" "$C"; done
grep -Eq 'draw_(test_pattern|scene)' "$C"
if grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' "$C"; then
  echo 'Stage-5A Phase-3 compositor must remain allocation-free/freestanding.' >&2; exit 1
fi
echo 'Stage-5A Phase-3 double-buffer/page-flip/ppoll/input render-loop contract: PASS'
