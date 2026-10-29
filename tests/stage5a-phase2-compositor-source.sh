#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"
for needle in \
  'NOVA_DRM_IOCTL_SET_MASTER' \
  'drm_master=yes' \
  'choose_connector_mode_crtc' \
  'count_modes == 0' \
  'connector_probe_failed' \
  'NOVA_DRM_MODE_CONNECTED' \
  'NOVA_DRM_MODE_TYPE_PREFERRED' \
  'NOVA_DRM_IOCTL_MODE_CREATE_DUMB' \
  'NOVA_DRM_IOCTL_MODE_ADDFB2' \
  'NOVA_DRM_FORMAT_XRGB8888' \
  'NOVA_DRM_IOCTL_MODE_MAP_DUMB' \
  'nova_mmap' \
  'NOVA_DRM_IOCTL_MODE_SETCRTC' \
  'NOVA_DRM_IOCTL_MODE_DIRTYFB' \
  'display_error_step=' \
  'display_error_errno=' \
  'create_dumb_failed' \
  'addfb2_failed' \
  'map_dumb_failed' \
  'mmap_dumb_failed' \
  'setcrtc_failed' \
  'display_stage=READY' \
  'modeset=ACTIVE' \
  'frame_presented=yes'; do
  grep -Fq "$needle" "$C"
done
grep -Eq 'draw_(test_pattern|scene)' "$C"
grep -Eq 'pattern=nova-(rgbw|scene)-v1' "$C"
python3 - "$C" <<'PY_MODES'
from pathlib import Path
import re,sys
s=Path(sys.argv[1]).read_text()
n=int(re.search(r'^#define NOVA_MAX_CONNECTOR_MODES (\d+)U$',s,re.M).group(1))
assert 32<=n<=128, 'connector mode inventory must remain bounded'
assert 'probe.count_modes > NOVA_MAX_CONNECTOR_MODES' in s
assert 'modes[NOVA_MAX_CONNECTOR_MODES]' in s
assert 'conn.count_modes = NOVA_MAX_CONNECTOR_MODES' in s
print('Connector mode bounded inventory and kernel count guard: PASS')
PY_MODES
grep -Fq 'pointer_is_error' "$C"
if grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' "$C"; then
  echo 'Stage-5A Phase-2 compositor must remain allocation-free/freestanding in this phase.' >&2
  exit 1
fi
echo 'Stage-5A Phase-2 DRM-master/force-probe/dumb-buffer/mmap/modeset diagnostic contract: PASS'
