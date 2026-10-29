#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
S="$ROOT/scripts/windows/final-stage5a-phase1-check.sh"
P="$ROOT/scripts/windows/stage5a-phase1-check.py"
for needle in 'NOVA_VERSION=0.0.12-dev' 'CONFIG_DRM_VIRTIO_GPU' 'prepare-ovmf-vars.sh' 'run-stage4d-package-suite.sh'; do grep -Fq "$needle" "$S"; done
for needle in "'-device','virtio-gpu-pci'" "'-device','virtio-keyboard-pci'" "'-device','virtio-mouse-pci'" 'OVMF Secure Boot + TPM + DRM/KMS + evdev runtime acceptance: PASS' 'drm_driver=virtio_gpu' "TPM_SOCK=Path('/tmp')" 'probe_deadline=time.monotonic()+20' "'cat /run/nova/graphics.state','services','state'"; do grep -Fq "$needle" "$P"; done
echo 'Stage-5A Phase-1 Secure Boot/TPM/DRM runtime acceptance source contract: PASS'
