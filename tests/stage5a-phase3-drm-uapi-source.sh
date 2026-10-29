#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
H="$ROOT/src/runtime/nova_drm.h"
I="$ROOT/src/runtime/nova_input.h"
for needle in \
  'struct nova_drm_mode_crtc_page_flip' \
  'struct nova_drm_event_vblank' \
  'NOVA_DRM_IOCTL_MODE_PAGE_FLIP' \
  'NOVA_DRM_MODE_PAGE_FLIP_EVENT' \
  'NOVA_DRM_EVENT_FLIP_COMPLETE' \
  '0xc01864b0UL'; do grep -Fq "$needle" "$H"; done
for needle in 'struct nova_input_event' 'NOVA_EV_KEY' 'NOVA_EV_REL' 'NOVA_KEY_SPACE' 'NOVA_REL_X' 'NOVA_REL_Y'; do grep -Fq "$needle" "$I"; done
echo 'Stage-5A Phase-3 page-flip/event + evdev UAPI ABI contract: PASS'
