#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIR="$ROOT/build/dev-keys"
KEY="$DIR/NOVA-DEVELOPMENT-ONLY.key.pem"
CERT_PEM="$DIR/NOVA-DEVELOPMENT-ONLY.crt.pem"
CERT_DER="$DIR/NOVA-DEVELOPMENT-ONLY.crt.der"
mkdir -p "$DIR"

if [[ ! -s "$KEY" || ! -s "$CERT_PEM" || ! -s "$CERT_DER" ]]; then
  echo "Generating a LOCAL DEVELOPMENT-ONLY NovaOS signing root..." >&2
  echo "This key is not a production trust anchor and must never be used for releases." >&2
  umask 077
  openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "$KEY" -out "$CERT_PEM" \
    -sha256 -days 3650 -set_serial 1 \
    -subj '/CN=NovaOS Local Development Root/O=NovaOS Development/C=TR' \
    -addext 'basicConstraints=critical,CA:FALSE' \
    -addext 'keyUsage=critical,digitalSignature' >/dev/null 2>&1
  openssl x509 -in "$CERT_PEM" -outform DER -out "$CERT_DER"
  chmod 0644 "$CERT_PEM" "$CERT_DER"
fi

# Re-assert permissions even when reusing an existing development key.
chmod 0600 "$KEY"
chmod 0644 "$CERT_PEM" "$CERT_DER"

printf '%s\n' "$CERT_DER"
