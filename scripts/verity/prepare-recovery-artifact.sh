#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ARCH="${1:-x86_64}"
OUT="$ROOT/build/$ARCH"
ART="$OUT/recovery-artifact"
SRC="$ART/source"
VERSION="${NOVA_VERSION:-0.0.12-dev}"

for tool in mke2fs veritysetup openssl sha256sum; do
  command -v "$tool" >/dev/null 2>&1 || { echo "ERROR: $tool is required to prepare the Stage-4D Recovery artifact." >&2; exit 2; }
done
if [[ -n "${NOVA_ROOT_CERT:-}" && -z "${NOVA_SIGNING_KEY:-}" ]]; then
  echo "ERROR: NOVA_ROOT_CERT was supplied without matching NOVA_SIGNING_KEY." >&2
  exit 2
fi
if [[ -n "${NOVA_SIGNING_KEY:-}" ]]; then
  SIGNING_KEY="$NOVA_SIGNING_KEY"
else
  "$ROOT/scripts/ensure-dev-key.sh" >/dev/null
  SIGNING_KEY="$ROOT/build/dev-keys/NOVA-DEVELOPMENT-ONLY.key.pem"
fi
[[ -s "$SIGNING_KEY" ]] || { echo "ERROR: NovaOS content-root signing key not found: $SIGNING_KEY" >&2; exit 2; }
ROOT_CERT_DER="$("$ROOT/scripts/prepare-trust.sh" "$ARCH")"

python3 "$ROOT/scripts/retain-build-files.py" "$ART"
mkdir -p "$SRC"
cp "$ROOT/configs/recovery/recovery.conf" "$SRC/recovery.conf"
cp "$ROOT/configs/recovery/README.txt" "$SRC/README.txt"

SALT="$(printf 'NovaOS Stage-4D recovery|0.0.11' | sha256sum | awk '{print $1}')"
"$ROOT/scripts/verity/build-recovery-slot.sh" \
  "$SRC" "$ART/recovery.slot" NOVA_RECOVERY \
  50daffd0-9420-5ba7-a77e-3dc68cbde31d "$SALT" "$ART/recovery.env"

# shellcheck disable=SC1090
source "$ART/recovery.env"
cat > "$ART/verity.manifest" <<MANIFEST
# NovaOS Stage-4D firmware-bound Recovery dm-verity descriptor
format=3
artifact_kind=recovery-slot
update_id=base-stage4d-recovery
version=0.0.11-base
rollback_index=0
payload=recovery.slot
payload_sha256=$payload_sha256
payload_size=$payload_size
verity_data_block_size=$verity_data_block_size
verity_hash_block_size=$verity_hash_block_size
verity_data_blocks=$verity_data_blocks
verity_hash_start_block=$verity_hash_start_block
verity_hash_algorithm=$verity_hash_algorithm
verity_root_hash=$verity_root_hash
verity_salt=$verity_salt
MANIFEST
"$ROOT/scripts/verity/validate-recovery-descriptor.py" "$ART/verity.manifest"
openssl dgst -sha256 -sign "$SIGNING_KEY" -out "$ART/verity.sig" "$ART/verity.manifest"
openssl x509 -inform DER -in "$ROOT_CERT_DER" -pubkey -noout > "$ART/root-public-key.pem"
openssl dgst -sha256 -verify "$ART/root-public-key.pem" -signature "$ART/verity.sig" "$ART/verity.manifest" >/dev/null
sha256sum "$ART/recovery.slot" "$ART/verity.manifest" "$ART/verity.sig" > "$ART/SHA256SUMS"
printf 'Prepared Stage-4D Recovery artifact for %s (%s): %s\n' "$ARCH" "$VERSION" "$ART" >&2
