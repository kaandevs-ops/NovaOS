#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH=x86_64
"$ROOT/scripts/build-userland.sh" "$ARCH" >/dev/null
"$ROOT/scripts/build-initramfs.sh" "$ARCH" >/dev/null
ROOTFS="$ROOT/build/$ARCH/rootfs"
MANIFEST="$ROOTFS/System/Integrity/manifest.sha256"
SIG="$ROOTFS/System/Integrity/manifest.sig"
CERT="$ROOTFS/System/Trust/nova-root.der"
PUB="$ROOT/build/$ARCH/test-root-public.pem"
[[ -s "$MANIFEST" && -s "$SIG" && -s "$CERT" ]]
openssl x509 -inform DER -in "$CERT" -pubkey -noout > "$PUB"
openssl dgst -sha256 -verify "$PUB" -signature "$SIG" "$MANIFEST" | grep -Fq 'Verified OK'
size="$(stat -c %s "$SIG")"
[[ "$size" -eq 256 ]]
echo "Signed manifest verification passed (RSA-2048 signature: ${size} bytes)."
