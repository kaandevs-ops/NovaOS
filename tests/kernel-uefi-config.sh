#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/configs/kernel/common.config"; X="$ROOT/configs/kernel/x86_64.config"; A="$ROOT/configs/kernel/arm64.config"; B="$ROOT/scripts/build-kernel.sh"
for sym in CONFIG_EFI CONFIG_EFI_STUB CONFIG_EFIVAR_FS; do grep -qx "$sym=y" "$C"; done
grep -qx 'CONFIG_CMDLINE_BOOL=y' "$X"
grep -qx 'CONFIG_CMDLINE_OVERRIDE=y' "$X"
grep -Fqx 'CONFIG_CMDLINE="console=ttyS0 rdinit=/init panic=-1 loglevel=4"' "$X"
grep -qx 'CONFIG_CMDLINE_FORCE=y' "$A"
grep -Fqx 'CONFIG_CMDLINE="console=ttyAMA0 rdinit=/init panic=-1 loglevel=4"' "$A"
for needle in 'INITRAMFS="$ROOT/build/$ARCH/initramfs.cpio"' '-e EFI -e EFI_STUB -e EFIVAR_FS' '--set-str INITRAMFS_SOURCE' 'CONFIG_EFI_STUB CONFIG_EFIVAR_FS'; do grep -Fq -- "$needle" "$B"; done
echo "Stage-4B EFI-stub + embedded-initramfs kernel config policy passed."
