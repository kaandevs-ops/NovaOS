#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/scripts/windows/final-tpm-check.py"
V="$ROOT/scripts/uefi/prepare-ovmf-vars.sh"
for needle in \
  "VARS_DISABLED=UEFI/'OVMF_VARS_SECUREBOOT_DISABLED_TEMPLATE.fd'" \
  'vars_src=VARS_SECURE if secure else VARS_DISABLED' \
  'CODE_SECURE' \
  "'-machine','q35,smm=on'" \
  "'-boot','order=c,menu=off,strict=on'" \
  'transcript saved to'; do grep -Fq -- "$needle" "$P"; done
for needle in \
  'OVMF_VARS_SECUREBOOT_DISABLED_TEMPLATE.fd' \
  '--set-false SecureBootEnable'; do grep -Fq -- "$needle" "$V"; done
echo 'Stage-4C Secure-Boot-disabled path source regression: PASS'
