#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/scripts/windows/stage5a-phase2-check.py"
S="$ROOT/scripts/windows/final-stage5a-phase2-check.sh"
for needle in \
  "'-device','virtio-gpu-pci,id=novagpu'" \
  "'-device','virtio-keyboard-pci'" \
  "'-device','virtio-mouse-pci'" \
  'screendump' \
  'verify_scanout_ppm' \
  'scanout pattern mismatch' \
  "b'[nova-compositor] frame_presented=yes'" \
  "'stage=DISPLAY_READY'" \
  "'modeset=ACTIVE'"; do
  grep -Fq "$needle" "$P"
done
grep -Fq "MON_SOCK=Path('/tmp')" "$P"
grep -Fq 'real KMS scanout/pixel proof' "$S"
grep -Fq 'stage5a-phase2-check.py' "$S"

# Pixel proof must use a real headless display surface and the intended virtio-gpu head.
grep -Fq "'-vga','none'" "$P"
grep -Fq "'-device','virtio-gpu-pci,id=novagpu'" "$P"
grep -Fq "'-display','none'" "$P"
grep -Fq "'-vnc','127.0.0.1:0,to=99'" "$P"
grep -Fq "screendump {SCREENSHOT} novagpu 0" "$P"
if grep -Fq "'-display','vnc=" "$P"; then
  echo 'Stage-5A Phase-2 acceptance must use QEMU -vnc, not an invented -display vnc backend' >&2
  exit 1
fi
grep -Fq "qemu-system-x86_64 -help" "$S"
grep -Fq -- "-vnc" "$S"
echo 'Stage-5A Phase-2 Secure Boot/TPM/KMS screenshot acceptance contract: PASS'
