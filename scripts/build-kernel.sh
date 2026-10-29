#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-arm64}"
SRC="$($ROOT/scripts/fetch-kernel.sh | tail -n1)"
LOCAL_SRC="$SRC"
if [[ -n "${NOVA_KERNEL_READONLY_REFERENCE:-}" ]]; then
  python3 "$ROOT/tools/r33-kernel-source-equivalence.py" "$LOCAL_SRC" "$NOVA_KERNEL_READONLY_REFERENCE" before
  SRC="$NOVA_KERNEL_READONLY_REFERENCE"
fi
OUT="$ROOT/build/$ARCH/kernel"
mkdir -p "$OUT"

case "$ARCH" in
  arm64)
    KARCH=arm64
    BASE_DEFCONFIG=defconfig
    IMAGE_REL=arch/arm64/boot/Image
    ;;
  x86_64)
    KARCH=x86_64
    BASE_DEFCONFIG=x86_64_defconfig
    IMAGE_REL=arch/x86/boot/bzImage
    ;;
  *) echo "Unsupported ARCH: $ARCH" >&2; exit 2 ;;
esac

MAKE=(make -C "$SRC" O="$OUT" ARCH="$KARCH")
"${MAKE[@]}" "$BASE_DEFCONFIG"
"$SRC/scripts/kconfig/merge_config.sh" -m -O "$OUT" \
  "$OUT/.config" \
  "$ROOT/configs/kernel/common.config" \
  "$ROOT/configs/kernel/$ARCH.config"
# Stage-4C binds the initramfs into the EFI-stub kernel image.  Secure Boot
# authenticates one PE/COFF object, so the initramfs cannot be replaced independently.
INITRAMFS="$ROOT/build/$ARCH/initramfs.cpio"
[[ -f "$INITRAMFS" ]] || { echo "ERROR: missing $INITRAMFS; run build-initramfs.sh first." >&2; exit 2; }
"$SRC/scripts/config" --file "$OUT/.config" -e EFI -e EFI_STUB -e EFIVAR_FS
"$SRC/scripts/config" --file "$OUT/.config" --set-str INITRAMFS_SOURCE "$INITRAMFS"
"$SRC/scripts/config" --file "$OUT/.config" --set-val INITRAMFS_ROOT_UID 0 --set-val INITRAMFS_ROOT_GID 0
case "$ARCH" in
  x86_64)
    "$SRC/scripts/config" --file "$OUT/.config" -e CMDLINE_BOOL -e CMDLINE_OVERRIDE
    "$SRC/scripts/config" --file "$OUT/.config" --set-str CMDLINE "console=ttyS0 rdinit=/init panic=-1 loglevel=4"
    ;;
  arm64)
    "$SRC/scripts/config" --file "$OUT/.config" -e CMDLINE_FORCE
    "$SRC/scripts/config" --file "$OUT/.config" --set-str CMDLINE "console=ttyAMA0 rdinit=/init panic=-1 loglevel=4"
    ;;
esac
"${MAKE[@]}" olddefconfig

for symbol in CONFIG_BLK_DEV_LOOP CONFIG_UNIX98_PTYS CONFIG_PID_NS CONFIG_NET_NS CONFIG_IPC_NS CONFIG_UTS_NS CONFIG_SECCOMP_FILTER CONFIG_KEYS CONFIG_ASYMMETRIC_KEY_TYPE CONFIG_ASYMMETRIC_PUBLIC_KEY_SUBTYPE CONFIG_X509_CERTIFICATE_PARSER CONFIG_CRYPTO_SHA256 CONFIG_CRYPTO_RSA CONFIG_VIRTIO_BLK CONFIG_EXT4_FS CONFIG_EFI_PARTITION CONFIG_BLK_DEV_DM CONFIG_DM_VERITY CONFIG_EFI CONFIG_EFI_STUB CONFIG_EFIVAR_FS CONFIG_TCG_TPM CONFIG_SYSFS CONFIG_SECURITYFS CONFIG_DEVTMPFS CONFIG_DEVTMPFS_MOUNT CONFIG_DRM CONFIG_DRM_KMS_HELPER CONFIG_DRM_VIRTIO_GPU CONFIG_DRM_VIRTIO_GPU_KMS CONFIG_INPUT CONFIG_INPUT_EVDEV CONFIG_VIRTIO_INPUT CONFIG_HID CONFIG_HID_GENERIC; do
  if ! grep -qx "${symbol}=y" "$OUT/.config"; then
    echo "ERROR: required NovaOS trust/storage/verity/UEFI/TPM/graphics symbol is not built-in: $symbol" >&2
    exit 3
  fi
done

if [[ "$ARCH" == "x86_64" ]]; then
  for symbol in CONFIG_ACPI CONFIG_PNP CONFIG_PNPACPI CONFIG_TCG_TIS_CORE CONFIG_TCG_TIS CONFIG_TCG_CRB; do
    if ! grep -qx "${symbol}=y" "$OUT/.config"; then
      echo "ERROR: required NovaOS Stage-4C TPM transport symbol is not built-in: $symbol" >&2
      exit 3
    fi
  done
fi

"${MAKE[@]}" -j"${JOBS:-$(nproc)}"

if [[ -n "${NOVA_KERNEL_READONLY_REFERENCE:-}" ]]; then
  python3 "$ROOT/tools/r33-kernel-source-equivalence.py" "$LOCAL_SRC" "$NOVA_KERNEL_READONLY_REFERENCE" after
fi
cp "$OUT/$IMAGE_REL" "$ROOT/build/$ARCH/kernel-image"
echo "Built: $ROOT/build/$ARCH/kernel-image"
