#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CFG="$ROOT/configs/kernel/common.config"
for symbol in CONFIG_KEYS CONFIG_ASYMMETRIC_KEY_TYPE CONFIG_ASYMMETRIC_PUBLIC_KEY_SUBTYPE CONFIG_X509_CERTIFICATE_PARSER CONFIG_CRYPTO_SHA256 CONFIG_CRYPTO_RSA; do
  grep -qx "${symbol}=y" "$CFG"
done
echo "Kernel trust configuration requirements are declared."
