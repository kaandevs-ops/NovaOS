#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-x86_64}"
if [[ -n "${NOVA_ROOT_CERT:-}" ]]; then
  CERT_SRC="$NOVA_ROOT_CERT"
else
  CERT_SRC="$("$ROOT/scripts/ensure-dev-key.sh")"
fi
OUT="$ROOT/build/$ARCH/trust"
DER="$OUT/nova-root.der"
mkdir -p "$OUT"

if [[ ! -f "$CERT_SRC" ]]; then
  echo "ERROR: NovaOS root certificate not found: $CERT_SRC" >&2
  exit 2
fi

# Normalize either PEM or DER input to the exact DER bytes pinned into /init.
if openssl x509 -inform DER -in "$CERT_SRC" -noout >/dev/null 2>&1; then
  cp "$CERT_SRC" "$DER"
elif openssl x509 -in "$CERT_SRC" -noout >/dev/null 2>&1; then
  openssl x509 -in "$CERT_SRC" -outform DER -out "$DER"
else
  echo "ERROR: NOVA_ROOT_CERT is not a readable X.509 PEM/DER certificate." >&2
  exit 2
fi

chmod 0644 "$DER"
echo "$DER"
