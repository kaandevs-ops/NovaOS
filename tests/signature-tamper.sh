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
TMP="$ROOT/build/$ARCH/tamper-test"
rm -rf "$TMP"; mkdir -p "$TMP"
openssl x509 -inform DER -in "$CERT" -pubkey -noout > "$TMP/public.pem"
cp "$MANIFEST" "$TMP/manifest.sha256"
cp "$SIG" "$TMP/manifest.sig"
printf '# tampered after signing\n' >> "$TMP/manifest.sha256"
if openssl dgst -sha256 -verify "$TMP/public.pem" -signature "$TMP/manifest.sig" "$TMP/manifest.sha256" >/dev/null 2>&1; then
  echo "ERROR: tampered manifest was accepted" >&2; exit 1
fi
cp "$MANIFEST" "$TMP/manifest.sha256"
python3 - "$TMP/manifest.sig" <<'PY'
import sys
p=sys.argv[1]
b=bytearray(open(p,'rb').read())
if not b: raise SystemExit(2)
b[len(b)//2] ^= 0x01
open(p,'wb').write(b)
PY
if openssl dgst -sha256 -verify "$TMP/public.pem" -signature "$TMP/manifest.sig" "$TMP/manifest.sha256" >/dev/null 2>&1; then
  echo "ERROR: tampered signature was accepted" >&2; exit 1
fi
echo "Signature negative tests passed (manifest tamper + signature tamper rejected)."
