#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-arm64}"
OUT="$ROOT/build/$ARCH"
KERNEL="$OUT/kernel-image"
INITRD="$OUT/initramfs.cpio.gz"
DISK="$OUT/novaos-stage4c.img"

[[ -f "$KERNEL" ]] || { echo "Missing $KERNEL; build first." >&2; exit 2; }
[[ -f "$INITRD" ]] || { echo "Missing $INITRD; build first." >&2; exit 2; }
[[ -f "$DISK" ]] || { echo "Missing $DISK; run ./scripts/build-disk.sh $ARCH first." >&2; exit 2; }

case "$ARCH" in
  arm64)
    exec qemu-system-aarch64 \
      -machine virt -cpu max -m 1024 -smp 4 \
      -kernel "$KERNEL" -initrd "$INITRD" \
      -drive "file=$DISK,if=virtio,format=raw,cache=writeback" \
      -append "console=ttyAMA0 rdinit=/init panic=-1 loglevel=4" \
      -nographic -no-reboot
    ;;
  x86_64)
    if [[ "${NOVA_DIRECT_BOOT:-0}" == 1 ]]; then
      echo "WARNING: direct -kernel developer boot does not satisfy Stage-4C firmware trust." >&2
      exec qemu-system-x86_64 -machine q35 -cpu max -m 1024 -smp 4 \
        -kernel "$KERNEL" -initrd "$INITRD" \
        -drive "file=$DISK,if=virtio,format=raw,cache=writeback" \
        -append "console=ttyS0 rdinit=/init panic=-1 loglevel=4" -nographic -no-reboot
    fi
    "$ROOT/scripts/uefi/prepare-ovmf-vars.sh" x86_64 >/dev/null
    UEFI="$OUT/uefi"; CODE="$(cat "$UEFI/ovmf-secure-code.path")"; VARS="$UEFI/OVMF_VARS_NOVA_RUN.fd"
    cp "$UEFI/OVMF_VARS_NOVA_TEMPLATE.fd" "$VARS"
    exec qemu-system-x86_64 -machine q35,smm=on -global driver=cfi.pflash01,property=secure,value=on \
      -cpu max -m 1024 -smp 4 \
      -drive "if=pflash,format=raw,unit=0,readonly=on,file=$CODE" \
      -drive "if=pflash,format=raw,unit=1,file=$VARS" \
      -drive "file=$DISK,if=virtio,format=raw,cache=writeback" \
      -nographic -no-reboot
    ;;
  *) echo "Unsupported ARCH: $ARCH" >&2; exit 2 ;;
esac
