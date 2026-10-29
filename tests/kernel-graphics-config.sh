#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CFG="$ROOT/configs/kernel/common.config"
for sym in CONFIG_DRM CONFIG_DRM_KMS_HELPER CONFIG_DRM_VIRTIO_GPU CONFIG_DRM_VIRTIO_GPU_KMS CONFIG_INPUT CONFIG_INPUT_EVDEV CONFIG_VIRTIO_INPUT CONFIG_HID CONFIG_HID_GENERIC; do
  grep -qx "$sym=y" "$CFG" || { echo "missing Stage-5A kernel symbol: $sym" >&2; exit 1; }
done
for sym in CONFIG_DRM CONFIG_DRM_KMS_HELPER CONFIG_DRM_VIRTIO_GPU CONFIG_DRM_VIRTIO_GPU_KMS CONFIG_INPUT CONFIG_INPUT_EVDEV CONFIG_VIRTIO_INPUT CONFIG_HID CONFIG_HID_GENERIC; do
  grep -Fq "$sym" "$ROOT/scripts/build-kernel.sh" || { echo "kernel build does not enforce $sym" >&2; exit 1; }
done
echo 'Stage-5A DRM/KMS + virtio-gpu + evdev/virtio-input kernel foundation: PASS'
