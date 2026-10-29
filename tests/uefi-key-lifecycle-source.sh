#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREP="$ROOT/scripts/uefi/prepare-uefi-trust.sh"
VARS="$ROOT/scripts/uefi/prepare-ovmf-vars.sh"
ESP="$ROOT/scripts/uefi/build-esp.sh"
INIT="$ROOT/scripts/build-initramfs.sh"
for needle in 'NOVA_UEFI_PK_CERT' 'NOVA_UEFI_KEK_CERT' 'NOVA_UEFI_DB_CERT' 'uefi-pk.crt.der' 'uefi-kek.crt.der' 'uefi-db.crt.der' 'dbx-policy.conf' 'production_key_ceremony=no'; do
  grep -Fq "$needle" "$PREP" || { echo "missing UEFI role contract: $needle" >&2; exit 1; }
done
for needle in '--set-pk "$GUID" "$PK_CERT"' '--add-kek "$GUID" "$KEK_CERT"' '--add-db "$GUID" "$DB_CERT"'; do
  grep -Fq -- "$needle" "$VARS" || { echo "missing OVMF role enrollment: $needle" >&2; exit 1; }
done
for needle in 'uefi-roles.conf' 'dbx-policy.conf' 'uefi-db.crt.der'; do grep -Fq "$needle" "$ESP"; done
for needle in 'pk_cert_sha256=' 'kek_cert_sha256=' 'db_signer_cert_sha256=' 'dbx_policy=foundation-empty' 'dbx_firmware_enrollment=not-performed-by-stage4d'; do grep -Fq "$needle" "$INIT"; done
# Stage-4D does not falsely claim actual dbx enrollment or production ceremony.
! grep -Eq -- '--(add|set)-dbx' "$VARS"
echo 'Stage-4D UEFI PK/KEK/db role separation + explicit dbx foundation source contract passed.'
