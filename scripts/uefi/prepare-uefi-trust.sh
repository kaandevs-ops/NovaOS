#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${1:-x86_64}"
OUT="$ROOT/build/$ARCH/uefi-trust"
mkdir -p "$OUT"

# Stage-4D keeps PK, KEK and db as separate roles even when the local
# development profile deliberately reuses one certificate for convenience.
# NOVA_UEFI_CERT remains a backward-compatible fallback for all three roles.
FALLBACK_CERT="${NOVA_UEFI_CERT:-}"
if [[ -z "$FALLBACK_CERT" ]] && { [[ -z "${NOVA_UEFI_PK_CERT:-}" ]] || [[ -z "${NOVA_UEFI_KEK_CERT:-}" ]] || [[ -z "${NOVA_UEFI_DB_CERT:-}" ]]; }; then
  FALLBACK_CERT="$("$ROOT/scripts/uefi/ensure-dev-uefi-key.sh")"
fi
PK_SRC="${NOVA_UEFI_PK_CERT:-$FALLBACK_CERT}"
KEK_SRC="${NOVA_UEFI_KEK_CERT:-$FALLBACK_CERT}"
DB_SRC="${NOVA_UEFI_DB_CERT:-$FALLBACK_CERT}"
for pair in "PK:$PK_SRC" "KEK:$KEK_SRC" "db:$DB_SRC"; do
  role="${pair%%:*}"; src="${pair#*:}"
  [[ -n "$src" && -f "$src" ]] || { echo "ERROR: UEFI $role certificate not found: $src" >&2; exit 2; }
done

normalize_cert() {
  local role="$1" src="$2" base="$3"
  local pem="$OUT/$base.crt.pem" der="$OUT/$base.crt.der"
  if openssl x509 -inform DER -in "$src" -noout >/dev/null 2>&1; then
    openssl x509 -inform DER -in "$src" -outform PEM -out "$pem"
    cp "$src" "$der"
  elif openssl x509 -in "$src" -noout >/dev/null 2>&1; then
    cp "$src" "$pem"
    openssl x509 -in "$src" -outform DER -out "$der"
  else
    echo "ERROR: UEFI $role certificate is not readable X.509 PEM/DER: $src" >&2
    exit 2
  fi
  chmod 0644 "$pem" "$der"
}

normalize_cert PK "$PK_SRC" uefi-pk
normalize_cert KEK "$KEK_SRC" uefi-kek
normalize_cert db "$DB_SRC" uefi-db

PK_SHA256="$(sha256sum "$OUT/uefi-pk.crt.der" | awk '{print $1}')"
KEK_SHA256="$(sha256sum "$OUT/uefi-kek.crt.der" | awk '{print $1}')"
DB_SHA256="$(sha256sum "$OUT/uefi-db.crt.der" | awk '{print $1}')"
ROLE_MODE=distinct-certificates
[[ "$PK_SHA256" == "$KEK_SHA256" && "$KEK_SHA256" == "$DB_SHA256" ]] && ROLE_MODE=shared-development-certificate

cat > "$OUT/uefi-roles.conf" <<ROLES
format=1
profile=${NOVA_UEFI_TRUST_PROFILE:-development}
pk_cert=/EFI/NOVA/uefi-pk.crt.der
pk_cert_sha256=$PK_SHA256
kek_cert=/EFI/NOVA/uefi-kek.crt.der
kek_cert_sha256=$KEK_SHA256
db_cert=/EFI/NOVA/uefi-db.crt.der
db_cert_sha256=$DB_SHA256
role_mode=$ROLE_MODE
production_key_ceremony=no
ROLES

# dbx is a separate revocation-policy role. Stage-4D deliberately does not
# fabricate a production revocation list or claim authenticated dbx rotation.
cat > "$OUT/dbx-policy.conf" <<DBX
format=1
role=dbx
mode=foundation-empty
entries=0
firmware_enrollment=not-performed-by-stage4d
production_revocation_ceremony=no
DBX

# Backward-compatible alias used by existing Stage-4B/4C scripts and runtime
# signer pin naming. Semantically this is the UEFI db image-signing certificate.
cp "$OUT/uefi-db.crt.pem" "$OUT/nova-uefi-root.crt.pem"
cp "$OUT/uefi-db.crt.der" "$OUT/nova-uefi-root.crt.der"
chmod 0644 "$OUT/nova-uefi-root.crt.pem" "$OUT/nova-uefi-root.crt.der" "$OUT/uefi-roles.conf" "$OUT/dbx-policy.conf"
printf '%s\n' "$OUT/uefi-db.crt.pem"
