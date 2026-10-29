#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
S="$ROOT/src/runtime/nova_syscall.h"
L="$ROOT/src/runtime/nova_linux.h"
D="$ROOT/src/runtime/nova_drm.h"
C="$ROOT/src/services/compositor.c"
B="$ROOT/scripts/build-userland.sh"
I="$ROOT/scripts/build-initramfs.sh"
for needle in 'nova_syscall6' 'nova_mmap' 'nova_munmap' 'nova_poll'; do grep -Fq "$needle" "$S"; done
for needle in 'NOVA_NR_mmap' 'NOVA_NR_munmap' 'NOVA_NR_ppoll' 'struct nova_pollfd'; do grep -Fq "$needle" "$L"; done
for needle in 'NOVA_DRM_IOCTL_VERSION' 'NOVA_DRM_IOCTL_MODE_GETRESOURCES' 'NOVA_EVIOCGVERSION' 'DRM KMS resources ABI size mismatch'; do grep -Fq "$needle" "$D"; done
for needle in '/dev/dri/card0' '/dev/input/event' 'stage=READY' 'drm_driver=' 'input_event_devices' 'NOVA_DRM_IOCTL_MODE_GETRESOURCES'; do grep -Fq "$needle" "$C"; done
grep -Fq 'src/services/compositor.c' "$B"
grep -Fq 'nova-compositor' "$B"
grep -Fq 'compositor.service' "$I"
echo 'Stage-5A native DRM/input runtime + compositor probe source contract: PASS'
