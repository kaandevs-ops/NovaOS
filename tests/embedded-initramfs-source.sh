#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; I="$ROOT/scripts/build-initramfs.sh"; K="$ROOT/scripts/build-kernel.sh"; E="$ROOT/scripts/uefi/build-esp.sh"
grep -Fq 'cat "$FILES_CPIO" "$DEVICES_CPIO" > "$OUT/initramfs.cpio"' "$I"
grep -Fq 'initramfs-device-cpio.py' "$I"
grep -Fq 'gzip -9n -c "$OUT/initramfs.cpio"' "$I"
grep -Fq -- '--set-str INITRAMFS_SOURCE "$INITRAMFS"' "$K"
grep -Fq 'initramfs_binding=builtin-in-efi-kernel' "$E"
echo "Stage-4B built-in initramfs binding source policy passed."
