#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/scripts/ensure-dev-key.sh"; U="$ROOT/scripts/uefi/ensure-dev-uefi-key.sh"; B="$ROOT/scripts/uefi/build-esp.sh"
grep -Fq 'NOVA-DEVELOPMENT-ONLY.key.pem' "$C"
grep -Fq 'NOVA-UEFI-DEVELOPMENT-ONLY.key.pem' "$U"
grep -Fq 'NovaOS Local Development UEFI Secure Boot' "$U"
grep -Fq 'NOVA_UEFI_SIGNING_KEY' "$B"
! grep -Fq 'NOVA-DEVELOPMENT-ONLY.key.pem' "$B"
echo "Stage-4B content-signing and UEFI-signing key separation policy passed."
