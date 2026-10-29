#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-x86_64}"
OUT="$ROOT/build/$ARCH"
ROOTFS="$OUT/rootfs"
STORAGE="$OUT/storage"
UPROOT="$STORAGE/update-root"
BUNDLE="$STORAGE/update-bundle"
UPDATE_ID="${NOVA_UPDATE_ID:-stage4d-demo-1}"
UPDATE_VERSION="${NOVA_UPDATE_VERSION:-0.0.11-demo1}"
ROLLBACK_INDEX="${NOVA_ROLLBACK_INDEX:-1}"
PAYLOAD="$BUNDLE/system.slot"
VERITY_META="$BUNDLE/system.verity.env"

command -v mke2fs >/dev/null 2>&1 || { echo "ERROR: mke2fs is required." >&2; exit 2; }
command -v veritysetup >/dev/null 2>&1 || { echo "ERROR: veritysetup is required (cryptsetup-bin)." >&2; exit 2; }
command -v openssl >/dev/null 2>&1 || { echo "ERROR: openssl is required." >&2; exit 2; }
[[ "$UPDATE_ID" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid NOVA_UPDATE_ID" >&2; exit 2; }
[[ "$UPDATE_VERSION" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid NOVA_UPDATE_VERSION" >&2; exit 2; }
[[ "$ROLLBACK_INDEX" =~ ^[0-9]+$ ]] || { echo "Invalid NOVA_ROLLBACK_INDEX" >&2; exit 2; }
python3 - "$ROLLBACK_INDEX" <<'PYUINT'
import sys
raw=sys.argv[1]
value=int(raw,10)
if (len(raw)>1 and raw[0]=='0') or value > (1<<64)-1:
    raise SystemExit("Invalid NOVA_ROLLBACK_INDEX: non-canonical or outside unsigned-long range")
PYUINT

if [[ ! -f "$ROOTFS/System/Integrity/manifest.sig" ]]; then
  "$ROOT/scripts/build-userland.sh" "$ARCH"
  "$ROOT/scripts/build-initramfs.sh" "$ARCH"
fi

python3 "$ROOT/scripts/retain-build-files.py" "$UPROOT" "$BUNDLE"
mkdir -p "$UPROOT/System" "$UPROOT/Recovery" "$BUNDLE"
cp -a "$ROOTFS/System/." "$UPROOT/System/"
cp -a "$ROOTFS/Recovery/." "$UPROOT/Recovery/"
mkdir -p "$UPROOT/System/Update"
printf '%s\n' "$UPDATE_ID" > "$UPROOT/System/Update/update-id"
printf '%s\n' "$UPDATE_VERSION" > "$UPROOT/System/Update/update-version"
printf 'NovaOS Stage-4D dm-verity protected updated System payload\n' > "$UPROOT/System/release"

python3 "$ROOT/scripts/retain-build-files.py" "$UPROOT/System/Integrity/manifest.sha256" "$UPROOT/System/Integrity/manifest.sig"
"$ROOT/scripts/generate-integrity-manifest.py" "$UPROOT"

if [[ -n "${NOVA_SIGNING_KEY:-}" ]]; then
  SIGNING_KEY="$NOVA_SIGNING_KEY"
else
  "$ROOT/scripts/ensure-dev-key.sh" >/dev/null
  SIGNING_KEY="$ROOT/build/dev-keys/NOVA-DEVELOPMENT-ONLY.key.pem"
fi
openssl dgst -sha256 -sign "$SIGNING_KEY" \
  -out "$UPROOT/System/Integrity/manifest.sig" \
  "$UPROOT/System/Integrity/manifest.sha256"

SALT="$(printf 'NovaOS Stage-4D update|%s|%s' "$UPDATE_ID" "$UPDATE_VERSION" | sha256sum | awk '{print $1}')"
"$ROOT/scripts/verity/build-system-slot.sh" \
  "$UPROOT/System" "$PAYLOAD" NOVA_UPDATE_SYSTEM \
  063d7c72-4b35-5ee8-b944-9985ed7921df "$SALT" "$VERITY_META"

# shellcheck disable=SC1090
source "$VERITY_META"
RELEASE_KEY_ID="${NOVA_UPDATE_SIGNER_KEY_ID:-${NOVA_RELEASE_KEY_ID:-nova-development-release-2026-a}}"
TRUST_EPOCH="${NOVA_UPDATE_TRUST_EPOCH:-${NOVA_TRUST_EPOCH:-1}}"
TRANSITION="${NOVA_UPDATE_TRANSITION:-none}"
[[ "$RELEASE_KEY_ID" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid NOVA_RELEASE_KEY_ID" >&2; exit 2; }
[[ "$TRUST_EPOCH" =~ ^[0-9]+$ ]] || { echo "Invalid NOVA_TRUST_EPOCH" >&2; exit 2; }
[[ "$TRANSITION" == none || "$TRANSITION" == promote-next ]] || { echo "Invalid NOVA_UPDATE_TRANSITION" >&2; exit 2; }
if [[ -n "${NOVA_UPDATE_SIGNING_KEY:-}" ]]; then
  RELEASE_SIGNING_KEY="$NOVA_UPDATE_SIGNING_KEY"
elif [[ -n "${NOVA_RELEASE_SIGNING_KEY:-}" ]]; then
  RELEASE_SIGNING_KEY="$NOVA_RELEASE_SIGNING_KEY"
else
  "$ROOT/scripts/ensure-dev-release-key.sh" >/dev/null
  RELEASE_SIGNING_KEY="$ROOT/build/dev-release-keys/NOVA-RELEASE-DEVELOPMENT-ONLY.key.pem"
fi
[[ -f "$RELEASE_SIGNING_KEY" ]] || { echo "ERROR: NovaOS release signing key not found: $RELEASE_SIGNING_KEY" >&2; exit 2; }

# Carry the target System release policy as a sidecar. The root signature
# authenticates the policy/certificate set; the delegated update signature
# below binds this exact policy digest to the payload being installed.
for f in release-policy.conf release-policy.sig release-active.der; do
  [[ -s "$UPROOT/System/Trust/$f" ]] || { echo "ERROR: target System trust sidecar missing: $f" >&2; exit 2; }
  cp "$UPROOT/System/Trust/$f" "$BUNDLE/$f"
done
python3 "$ROOT/scripts/retain-build-files.py" "$BUNDLE/release-next.der"
[[ -s "$UPROOT/System/Trust/release-next.der" ]] && cp "$UPROOT/System/Trust/release-next.der" "$BUNDLE/release-next.der"
TARGET_POLICY_SHA256="$(sha256sum "$BUNDLE/release-policy.conf" | awk '{print $1}')"
ROOT_CERT="$UPROOT/System/Trust/nova-root.der"
[[ -s "$ROOT_CERT" ]] || { echo "ERROR: target System content-root certificate missing" >&2; exit 2; }
openssl x509 -inform DER -in "$ROOT_CERT" -pubkey -noout > "$STORAGE/update-root-public-key.pem"
openssl dgst -sha256 -verify "$STORAGE/update-root-public-key.pem" \
  -signature "$BUNDLE/release-policy.sig" "$BUNDLE/release-policy.conf" >/dev/null
python3 "$ROOT/scripts/validate-release-policy.py" \
  "$BUNDLE/release-policy.conf" --expect-epoch "$TRUST_EPOCH" \
  --expect-active "$RELEASE_KEY_ID" >/dev/null

# A promote-next bundle must prove lineage from the currently authorized
# root-signed policy. Runtime verification remains authoritative, but rejecting
# a broken transition here prevents producing a cryptographically valid yet
# semantically unusable release artifact.
if [[ "$TRANSITION" == promote-next ]]; then
  PREVIOUS_POLICY="${NOVA_PREVIOUS_RELEASE_POLICY:-}"
  [[ -n "$PREVIOUS_POLICY" && -s "$PREVIOUS_POLICY" ]] || { echo "ERROR: promote-next requires NOVA_PREVIOUS_RELEASE_POLICY" >&2; exit 2; }
  PREVIOUS_POLICY_SIG="${NOVA_PREVIOUS_RELEASE_POLICY_SIGNATURE:-$(dirname "$PREVIOUS_POLICY")/release-policy.sig}"
  [[ -s "$PREVIOUS_POLICY_SIG" ]] || { echo "ERROR: previous release-policy signature missing: $PREVIOUS_POLICY_SIG" >&2; exit 2; }
  openssl dgst -sha256 -verify "$STORAGE/update-root-public-key.pem" \
    -signature "$PREVIOUS_POLICY_SIG" "$PREVIOUS_POLICY" >/dev/null || { echo "ERROR: previous release policy is not authenticated by the target content root" >&2; exit 2; }

  python3 "$ROOT/scripts/validate-release-policy.py" \
    "$BUNDLE/release-policy.conf" --expect-epoch "$TRUST_EPOCH" \
    --expect-active "$RELEASE_KEY_ID" --previous "$PREVIOUS_POLICY" \
    --promote-next >/dev/null
fi

cat > "$BUNDLE/update.manifest" <<MANIFEST
# NovaOS Stage-4D delegated-release signed inactive-slot update + dm-verity descriptor
format=5
artifact_kind=system-update
update_id=$UPDATE_ID
version=$UPDATE_VERSION
rollback_index=$ROLLBACK_INDEX
trust_epoch=$TRUST_EPOCH
signer_key_id=$RELEASE_KEY_ID
transition=$TRANSITION
target_release_policy_sha256=$TARGET_POLICY_SHA256
payload=system.slot
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
openssl dgst -sha256 -sign "$RELEASE_SIGNING_KEY" \
  -out "$BUNDLE/update.sig" "$BUNDLE/update.manifest"

# Host-side self-verification uses the target policy's active certificate.
# Rotation updates therefore prove that the promoted key actually signed the
# target slot descriptor, while the currently running policy must separately
# authorize that key as `next` before apply.
openssl x509 -inform DER -in "$BUNDLE/release-active.der" -pubkey -noout > "$STORAGE/update-release-public-key.pem"
openssl dgst -sha256 -verify "$STORAGE/update-release-public-key.pem" \
  -signature "$BUNDLE/update.sig" "$BUNDLE/update.manifest" >/dev/null

printf 'Built signed Stage-4D verity update bundle: %s\n' "$BUNDLE"
printf '  id=%s version=%s rollback_index=%s trust_epoch=%s signer=%s transition=%s size=%s sha256=%s\n' "$UPDATE_ID" "$UPDATE_VERSION" "$ROLLBACK_INDEX" "$TRUST_EPOCH" "$RELEASE_KEY_ID" "$TRANSITION" "$payload_size" "$payload_sha256"
printf '  target_policy_sha256=%s verity_root=%s salt=%s\n' "$TARGET_POLICY_SHA256" "$verity_root_hash" "$verity_salt"
