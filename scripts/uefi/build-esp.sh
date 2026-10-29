#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${1:-x86_64}"
MODE="${2:-signed}"
OUT="$ROOT/build/$ARCH"
UEFI="$OUT/uefi"
KERNEL="$OUT/kernel-image"
mkdir -p "$UEFI"

for tool in openssl mkfs.vfat mmd mcopy python3; do
  command -v "$tool" >/dev/null 2>&1 || { echo "ERROR: $tool is required to build the Stage-4C ESP." >&2; exit 2; }
done
[[ -f "$KERNEL" ]] || { echo "ERROR: missing EFI-stub kernel image: $KERNEL" >&2; exit 2; }

case "$ARCH" in
  x86_64) EFI_NAME=BOOTX64.EFI ;;
  arm64) EFI_NAME=BOOTAA64.EFI ;;
  *) echo "Unsupported ARCH: $ARCH" >&2; exit 2 ;;
esac

CERT_PEM="$("$ROOT/scripts/uefi/prepare-uefi-trust.sh" "$ARCH")"
CERT_DER="$OUT/uefi-trust/uefi-db.crt.der"
CERT_SHA256="$(sha256sum "$CERT_DER" | awk '{print $1}')"
ARTIFACT="$UEFI/$EFI_NAME"
[[ "$MODE" == unsigned ]] && ARTIFACT="$UEFI/UNSIGNED-$EFI_NAME"
[[ "$MODE" == tampered ]] && ARTIFACT="$UEFI/TAMPERED-$EFI_NAME"

if [[ "$MODE" == signed ]]; then
  command -v sbsign >/dev/null 2>&1 || { echo "ERROR: sbsign is required for Stage-4C signed EFI images." >&2; exit 2; }
  command -v sbverify >/dev/null 2>&1 || { echo "ERROR: sbverify is required for Stage-4C signed EFI images." >&2; exit 2; }
  if [[ -n "${NOVA_UEFI_SIGNING_KEY:-}" ]]; then KEY="$NOVA_UEFI_SIGNING_KEY"; else
    "$ROOT/scripts/uefi/ensure-dev-uefi-key.sh" >/dev/null
    KEY="$ROOT/build/dev-keys/uefi/NOVA-UEFI-DEVELOPMENT-ONLY.key.pem"
  fi
  [[ -f "$KEY" ]] || { echo "ERROR: UEFI signing key not found: $KEY" >&2; exit 2; }
  key_pub="$UEFI/key.pub.pem"; cert_pub="$UEFI/cert.pub.pem"
  openssl pkey -in "$KEY" -pubout -out "$key_pub" >/dev/null 2>&1
  openssl x509 -in "$CERT_PEM" -pubkey -noout > "$cert_pub"
  cmp -s "$key_pub" "$cert_pub" || { echo "ERROR: UEFI image-signing key does not match the configured UEFI db certificate." >&2; exit 2; }
  python3 "$ROOT/scripts/retain-build-files.py" "$key_pub" "$cert_pub"
  sbsign --key "$KEY" --cert "$CERT_PEM" --output "$ARTIFACT" "$KERNEL" >/dev/null
  sbverify --cert "$CERT_PEM" "$ARTIFACT" >/dev/null
elif [[ "$MODE" == unsigned ]]; then
  cp "$KERNEL" "$ARTIFACT"
elif [[ "$MODE" == tampered ]]; then
  [[ -f "$UEFI/$EFI_NAME" ]] || "$ROOT/scripts/uefi/build-esp.sh" "$ARCH" signed >/dev/null
  cp "$UEFI/$EFI_NAME" "$ARTIFACT"
  python3 - "$ARTIFACT" <<'PYTAMPER'
from pathlib import Path
import sys
p=Path(sys.argv[1]); b=bytearray(p.read_bytes())
if len(b)<16384: raise SystemExit('EFI image unexpectedly small')
pos=len(b)//2; b[pos]^=0x5a; p.write_bytes(b)
PYTAMPER
else
  echo "usage: $0 <x86_64|arm64> [signed|unsigned|tampered]" >&2; exit 2
fi

cat > "$UEFI/boot-policy.conf" <<POLICY
format=1
stage=4D
arch=$ARCH
efi_fallback_path=EFI/BOOT/$EFI_NAME
secure_boot_required=yes
initramfs_binding=builtin-in-efi-kernel
signer_cert_sha256=$CERT_SHA256
image_sha256=$(sha256sum "$ARTIFACT" | awk '{print $1}')
POLICY

ESP="$UEFI/esp-${MODE}.fat"
python3 "$ROOT/scripts/retain-build-files.py" "$ESP"
ESP_SIZE="$(python3 "$ROOT/tools/esp-size.py" "$ARTIFACT")"
truncate -s "$ESP_SIZE" "$ESP"
mkfs.vfat -F 32 -n NOVA_ESP "$ESP" >/dev/null
MTOOLS_SKIP_CHECK=1 mmd -i "$ESP" ::/EFI ::/EFI/BOOT ::/EFI/NOVA
MTOOLS_SKIP_CHECK=1 mcopy -i "$ESP" "$ARTIFACT" "::/EFI/BOOT/$EFI_NAME"
MTOOLS_SKIP_CHECK=1 mcopy -i "$ESP" "$UEFI/boot-policy.conf" ::/EFI/NOVA/boot-policy.conf
MTOOLS_SKIP_CHECK=1 mcopy -i "$ESP" "$CERT_DER" ::/EFI/NOVA/uefi-db.crt.der
MTOOLS_SKIP_CHECK=1 mcopy -i "$ESP" "$OUT/uefi-trust/uefi-pk.crt.der" ::/EFI/NOVA/uefi-pk.crt.der
MTOOLS_SKIP_CHECK=1 mcopy -i "$ESP" "$OUT/uefi-trust/uefi-kek.crt.der" ::/EFI/NOVA/uefi-kek.crt.der
MTOOLS_SKIP_CHECK=1 mcopy -i "$ESP" "$OUT/uefi-trust/uefi-roles.conf" ::/EFI/NOVA/uefi-roles.conf
MTOOLS_SKIP_CHECK=1 mcopy -i "$ESP" "$OUT/uefi-trust/dbx-policy.conf" ::/EFI/NOVA/dbx-policy.conf
# Compatibility alias: this is the db signer certificate, not a PK/KEK root.
MTOOLS_SKIP_CHECK=1 mcopy -i "$ESP" "$CERT_DER" ::/EFI/NOVA/nova-uefi-root.crt.der
sha256sum "$ESP" > "$ESP.sha256"
echo "Built Stage-4D $MODE ESP: $ESP"
