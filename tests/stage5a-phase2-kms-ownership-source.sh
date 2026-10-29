#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"
K="$ROOT/configs/kernel/common.config"
B="$ROOT/scripts/build-kernel.sh"
grep -Fq 'NOVA_DRM_IOCTL_SET_MASTER' "$C"
grep -Fq 'probe.connector_id = connector_ids[ci]' "$C"
grep -Fq 'count_modes == 0' "$C"
grep -Fq 'conn.count_modes = NOVA_MAX_CONNECTOR_MODES' "$C"
grep -Fq 'NOVA_DRM_IOCTL_MODE_ADDFB2' "$C"
grep -Fq 'display_error_errno=' "$C"
grep -Fxq 'CONFIG_DRM_VIRTIO_GPU_KMS=y' "$K"
grep -Fq 'CONFIG_DRM_VIRTIO_GPU_KMS' "$B"
echo 'Stage-5A Phase-2 explicit DRM ownership + connector refresh + virtio KMS regression: PASS'
