#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIR="$ROOT/build/dev-release-keys"
KEY="$DIR/NOVA-RELEASE-DEVELOPMENT-ONLY.key.pem"
CERT_PEM="$DIR/NOVA-RELEASE-DEVELOPMENT-ONLY.crt.pem"
CERT_DER="$DIR/NOVA-RELEASE-DEVELOPMENT-ONLY.crt.der"
mkdir -p "$DIR"

if [[ ! -s "$KEY" || ! -s "$CERT_PEM" || ! -s "$CERT_DER" ]]; then
  echo "Generating a LOCAL DEVELOPMENT-ONLY NovaOS delegated release signer..." >&2
  echo "The certificate is authenticated by the root-signed System trust manifest, not treated as an independent root." >&2
  umask 077
  openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "$KEY" -out "$CERT_PEM" \
    -sha256 -days 3650 -set_serial 41 \
    -subj '/CN=NovaOS Local Development Release Signer/O=NovaOS Development/C=TR' \
    -addext 'basicConstraints=critical,CA:FALSE' \
    -addext 'keyUsage=critical,digitalSignature' >/dev/null 2>&1
  openssl x509 -in "$CERT_PEM" -outform DER -out "$CERT_DER"
fi
chmod 0600 "$KEY"
chmod 0644 "$CERT_PEM" "$CERT_DER"
printf '%s\n' "$CERT_DER"
