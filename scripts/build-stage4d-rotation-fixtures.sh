#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-x86_64}"; OUT="$ROOT/build/$ARCH"; STORAGE="$OUT/storage"; FIX="$OUT/stage4d-fixtures"
A_ID="${NOVA_STAGE4D_ACTIVE_A_ID:-nova-development-release-2026-a}"
B_ID="${NOVA_STAGE4D_NEXT_B_ID:-nova-development-release-2026-b}"
[[ -d "$STORAGE/update-bundle" && -d "$STORAGE/staging/system" ]] || { echo "ERROR: build base Stage-4D disk first" >&2; exit 2; }
python3 "$ROOT/scripts/retain-build-files.py" "$FIX"; mkdir -p "$FIX/base-update" "$FIX/base-system" "$FIX/rotation-update" "$FIX/stale-b-update" "$FIX/stale-policy" "$FIX/corrupt-policy" "$FIX/missing-active-key"
cp -a "$STORAGE/update-bundle/." "$FIX/base-update/"
cp -a "$STORAGE/staging/system/." "$FIX/base-system/"

# The initial policy must explicitly authorize B as next.
grep -qx 'format=3' "$FIX/base-system/Trust/release-policy.conf"
grep -qx 'trust_epoch=1' "$FIX/base-system/Trust/release-policy.conf"
grep -qx "active_key_id=$A_ID" "$FIX/base-system/Trust/release-policy.conf"
grep -qx "next_key_id=$B_ID" "$FIX/base-system/Trust/release-policy.conf"

B_CERT="$ROOT/build/dev-release-keys/NOVA-RELEASE-NEXT-DEVELOPMENT-ONLY.crt.der"
B_KEY="$ROOT/build/dev-release-keys/NOVA-RELEASE-NEXT-DEVELOPMENT-ONLY.key.pem"
[[ -s "$B_CERT" && -s "$B_KEY" ]] || "$ROOT/scripts/ensure-dev-release-next-key.sh" >/dev/null

# Build the target System trust state: B becomes active, A becomes revoked,
# epoch advances exactly once. EFI/kernel are intentionally NOT rebuilt here.
NOVA_RELEASE_KEY_ID="$B_ID" \
NOVA_RELEASE_CERT="$B_CERT" \
NOVA_RELEASE_NEXT_KEY_ID=none \
NOVA_RELEASE_REVOKED_KEY_IDS="$A_ID" \
NOVA_TRUST_EPOCH=2 \
  "$ROOT/scripts/build-initramfs.sh" "$ARCH"

NOVA_UPDATE_ID=stage4d-rotation-2 \
NOVA_UPDATE_VERSION=0.0.11-rotation2 \
NOVA_ROLLBACK_INDEX=1 \
NOVA_UPDATE_SIGNER_KEY_ID="$B_ID" \
NOVA_UPDATE_TRUST_EPOCH=2 \
NOVA_UPDATE_TRANSITION=promote-next \
NOVA_UPDATE_SIGNING_KEY="$B_KEY" \
NOVA_RELEASE_KEY_ID="$B_ID" \
NOVA_TRUST_EPOCH=2 \
NOVA_PREVIOUS_RELEASE_POLICY="$FIX/base-system/Trust/release-policy.conf" \
NOVA_PREVIOUS_RELEASE_POLICY_SIGNATURE="$FIX/base-system/Trust/release-policy.sig" \
  "$ROOT/scripts/build-update-bundle.sh" "$ARCH"
cp -a "$STORAGE/update-bundle/." "$FIX/rotation-update/"

grep -qx 'trust_epoch=2' "$FIX/rotation-update/update.manifest"
grep -qx "signer_key_id=$B_ID" "$FIX/rotation-update/update.manifest"
grep -qx 'transition=promote-next' "$FIX/rotation-update/update.manifest"

# Build a cryptographically valid B-signed same-policy update that intentionally
# carries rollback_index=1. After B is confirmed and the TPM counter is 1, this
# fixture still verifies under current trust but must be rejected by Stage-4C
# anti-rollback because the next accepted index is 2.
NOVA_UPDATE_ID=stage4d-stale-b-1 \
NOVA_UPDATE_VERSION=0.0.11-stale-b1 \
NOVA_ROLLBACK_INDEX=1 \
NOVA_UPDATE_SIGNER_KEY_ID="$B_ID" \
NOVA_UPDATE_TRUST_EPOCH=2 \
NOVA_UPDATE_TRANSITION=none \
NOVA_UPDATE_SIGNING_KEY="$B_KEY" \
NOVA_RELEASE_KEY_ID="$B_ID" \
NOVA_TRUST_EPOCH=2 \
  "$ROOT/scripts/build-update-bundle.sh" "$ARCH"
cp -a "$STORAGE/update-bundle/." "$FIX/stale-b-update/"
grep -qx 'format=5' "$FIX/stale-b-update/update.manifest"
grep -qx 'rollback_index=1' "$FIX/stale-b-update/update.manifest"
grep -qx 'trust_epoch=2' "$FIX/stale-b-update/update.manifest"
grep -qx "signer_key_id=$B_ID" "$FIX/stale-b-update/update.manifest"
grep -qx 'transition=none' "$FIX/stale-b-update/update.manifest"

ROOT_KEY="${NOVA_SIGNING_KEY:-$ROOT/build/dev-keys/NOVA-DEVELOPMENT-ONLY.key.pem}"
[[ -s "$ROOT_KEY" ]] || { echo "ERROR: missing root signing key for Stage-4D fixtures" >&2; exit 2; }

build_system_fixture() {
  local name="$1" source_system="$2" update_id="$3" version="$4" rollback_index="$5" salt_text="$6"
  local dir srcroot envfile label
  dir="$FIX/$name"
  srcroot="$FIX/.${name}-root"
  envfile="$dir/system.env"
  case "$name" in
    stale-policy) label=N4D_STALEPOL ;;
    corrupt-policy) label=N4D_BADPOL ;;
    missing-active-key) label=N4D_MISSKEY ;;
    *) label=N4D_FIXTURE ;;
  esac
  python3 "$ROOT/scripts/retain-build-files.py" "$srcroot"; mkdir -p "$srcroot/System"
  cp -a "$source_system/." "$srcroot/System/"
  local salt; salt="$(printf '%s' "$salt_text" | sha256sum | awk '{print $1}')"
  "$ROOT/scripts/verity/build-system-slot.sh" "$srcroot/System" "$dir/system.slot" "$label" \
    9b3541d8-47c0-51eb-9440-2d9cbfa87fb4 "$salt" "$envfile"
  # shellcheck disable=SC1090
  source "$envfile"
  cat > "$dir/verity.manifest" <<MANIFEST
# Stage-4D controlled negative System fixture: $name
format=3
artifact_kind=system-slot
update_id=$update_id
version=$version
rollback_index=$rollback_index
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
  openssl dgst -sha256 -sign "$ROOT_KEY" -out "$dir/verity.sig" "$dir/verity.manifest"
  python3 "$ROOT/scripts/retain-build-files.py" "$srcroot"
}

# Root-signed/dm-verity-valid System carrying the OLD epoch-1 policy, but with
# rollback_index=1. After trust epoch commit this isolates trust-policy
# anti-rollback from the Stage-4C System rollback-index check.
build_system_fixture stale-policy "$FIX/base-system" stage4d-stale-policy 0.0.11-stale-policy 1 'NovaOS Stage-4D stale trust policy fixture'

# Two epoch-2 negative fixtures keep System integrity + dm-verity valid while
# breaking only delegated-policy authentication/availability.  Keep a real
# rootfs layout here: generate-integrity-manifest.py intentionally accepts the
# parent containing System/, while build_system_fixture accepts System/ itself.
build_negative_system_fixture() {
  local name="$1" mutation="$2" update_id="$3" version="$4" rollback_index="$5" salt_text="$6"
  local negroot system
  negroot="$FIX/.${name}-negative-root"
  system="$negroot/System"
  python3 "$ROOT/scripts/retain-build-files.py" "$negroot"
  mkdir -p "$system"
  cp -a "$OUT/rootfs/System/." "$system/"

  case "$mutation" in
    corrupt-policy)
      printf '# controlled detached-policy corruption\n' >> "$system/Trust/release-policy.conf"
      ;;
    missing-active-key)
      python3 "$ROOT/scripts/retain-build-files.py" "$system/Trust/release-active.der"
      ;;
    *)
      echo "ERROR: unknown Stage-4D negative fixture mutation: $mutation" >&2
      return 2
      ;;
  esac

  python3 "$ROOT/scripts/retain-build-files.py" "$system/Integrity/manifest.sha256" "$system/Integrity/manifest.sig"
  "$ROOT/scripts/generate-integrity-manifest.py" "$negroot"
  [[ -s "$system/Integrity/manifest.sha256" ]] || { echo "ERROR: negative fixture integrity manifest missing: $name" >&2; return 2; }
  grep -Fq '  /System/' "$system/Integrity/manifest.sha256" || { echo "ERROR: negative fixture integrity manifest has no System entries: $name" >&2; return 2; }
  openssl dgst -sha256 -sign "$ROOT_KEY" -out "$system/Integrity/manifest.sig" "$system/Integrity/manifest.sha256"
  build_system_fixture "$name" "$system" "$update_id" "$version" "$rollback_index" "$salt_text"
  python3 "$ROOT/scripts/retain-build-files.py" "$negroot"
}

build_negative_system_fixture corrupt-policy corrupt-policy \
  stage4d-corrupt-policy 0.0.11-corrupt-policy 1 'NovaOS Stage-4D corrupt release policy fixture'
build_negative_system_fixture missing-active-key missing-active-key \
  stage4d-missing-active-key 0.0.11-missing-key 1 'NovaOS Stage-4D missing delegated active key fixture'

echo "Stage-4D rotation/failure fixtures built: $FIX"
