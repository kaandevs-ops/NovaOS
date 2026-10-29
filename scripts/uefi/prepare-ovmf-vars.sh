#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${1:-x86_64}"
[[ "$ARCH" == x86_64 ]] || { echo "Stage-4D OVMF acceptance currently targets x86_64." >&2; exit 2; }
command -v virt-fw-vars >/dev/null 2>&1 || { echo "ERROR: virt-fw-vars is required (Ubuntu package: python3-virt-firmware)." >&2; exit 2; }
OUT="$ROOT/build/$ARCH/uefi"
mkdir -p "$OUT"
DB_CERT="$("$ROOT/scripts/uefi/prepare-uefi-trust.sh" "$ARCH")"
PK_CERT="$ROOT/build/$ARCH/uefi-trust/uefi-pk.crt.pem"
KEK_CERT="$ROOT/build/$ARCH/uefi-trust/uefi-kek.crt.pem"
GUID="9b1f2f60-7e56-5ef7-8a58-ea2f28de4e32"

find_one() { for p in "$@"; do [[ -f "$p" ]] && { printf '%s\n' "$p"; return 0; }; done; return 1; }
CODE_SECURE="$(find_one /usr/share/OVMF/OVMF_CODE_4M.secboot.fd /usr/share/OVMF/OVMF_CODE.secboot.fd)" || { echo "ERROR: OVMF Secure Boot firmware not found." >&2; exit 2; }
CODE_PLAIN="$(find_one /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd)" || { echo "ERROR: OVMF firmware not found." >&2; exit 2; }
VARS_BASE="$(find_one /usr/share/OVMF/OVMF_VARS_4M.fd /usr/share/OVMF/OVMF_VARS.fd)" || { echo "ERROR: OVMF variable template not found." >&2; exit 2; }

ENROLLED="$OUT/OVMF_VARS_NOVA_TEMPLATE.fd"
virt-fw-vars --input "$VARS_BASE" --output "$ENROLLED" \
  --set-pk "$GUID" "$PK_CERT" --add-kek "$GUID" "$KEK_CERT" --add-db "$GUID" "$DB_CERT" --secure-boot >/dev/null
DISABLED="$OUT/OVMF_VARS_SECUREBOOT_DISABLED_TEMPLATE.fd"
virt-fw-vars --input "$ENROLLED" --output "$DISABLED" --set-false SecureBootEnable >/dev/null
printf '%s\n' "$CODE_SECURE" > "$OUT/ovmf-secure-code.path"
printf '%s\n' "$CODE_PLAIN" > "$OUT/ovmf-plain-code.path"
virt-fw-vars --input "$ENROLLED" --print > "$OUT/ovmf-vars.txt"
grep -q 'SecureBoot' "$OUT/ovmf-vars.txt" || { echo "ERROR: generated OVMF vars do not contain SecureBoot state." >&2; exit 3; }
echo "Prepared NovaOS Stage-4D OVMF Secure Boot variable store (PK/KEK/db roles explicit; dbx foundation remains unenrolled): $ENROLLED"
