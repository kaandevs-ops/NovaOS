#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
D="$ROOT/src/runtime/nova_drm.h"
for needle in \
  'struct nova_drm_mode_modeinfo' \
  'struct nova_drm_mode_crtc' \
  'struct nova_drm_mode_get_encoder' \
  'struct nova_drm_mode_get_connector' \
  'struct nova_drm_mode_fb_cmd2' \
  'struct nova_drm_mode_create_dumb' \
  'struct nova_drm_mode_map_dumb' \
  'struct nova_drm_mode_fb_dirty_cmd' \
  'NOVA_DRM_IOCTL_SET_MASTER' \
  'NOVA_DRM_IOCTL_MODE_GETCONNECTOR' \
  'NOVA_DRM_IOCTL_MODE_GETENCODER' \
  'NOVA_DRM_IOCTL_MODE_SETCRTC' \
  'NOVA_DRM_IOCTL_MODE_ADDFB2' \
  'NOVA_DRM_FORMAT_XRGB8888' \
  'NOVA_DRM_IOCTL_MODE_CREATE_DUMB' \
  'NOVA_DRM_IOCTL_MODE_MAP_DUMB' \
  'NOVA_DRM_IOCTL_MODE_DIRTYFB'; do
  grep -Fq "$needle" "$D"
done
for abi in \
  '0x641eUL' \
  '0xc06864a2UL' \
  '0xc01464a6UL' \
  '0xc05064a7UL' \
  '0xc06864b8UL' \
  '0x34325258U' \
  '0xc01864b1UL' \
  '0xc02064b2UL' \
  '0xc01064b3UL'; do
  grep -Fq "$abi" "$D"
done
echo 'Stage-5A Phase-2 DRM master + explicit XRGB8888 ADDFB2 UAPI ABI contract: PASS'
