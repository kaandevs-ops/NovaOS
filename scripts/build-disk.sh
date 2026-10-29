#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-x86_64}"
OUT="$ROOT/build/$ARCH"
ROOTFS="$OUT/rootfs"
STORAGE="$OUT/storage"
DISK="$OUT/novaos-stage4c.img"
VERSION="${NOVA_VERSION:-0.0.12-dev}"

for tool in mke2fs veritysetup python3 openssl; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "ERROR: $tool is required for Stage-4D disk images." >&2
    echo "Run ./scripts/windows/setup-wsl.sh (installs e2fsprogs + cryptsetup-bin)." >&2
    exit 2
  }
done

RECOVERY_ARTIFACT="$OUT/recovery-artifact"
if [[ ! -x "$ROOTFS/init" || ! -f "$ROOTFS/System/Integrity/manifest.sig" || ! -f "$OUT/initramfs.cpio" || \
      ! -s "$RECOVERY_ARTIFACT/recovery.slot" || ! -s "$RECOVERY_ARTIFACT/verity.manifest" || ! -s "$RECOVERY_ARTIFACT/verity.sig" || \
      ! -s "$ROOTFS/Trust/recovery-verity.manifest" || ! -s "$ROOTFS/Trust/recovery-verity.sig" ]]; then
  NOVA_VERSION="$VERSION" "$ROOT/scripts/build-userland.sh" "$ARCH"
  NOVA_VERSION="$VERSION" "$ROOT/scripts/build-initramfs.sh" "$ARCH"
fi
if [[ ! -f "$OUT/kernel-image" ]]; then NOVA_VERSION="$VERSION" "$ROOT/scripts/build-kernel.sh" "$ARCH"; fi
if [[ ! -f "$OUT/uefi/esp-signed.fat" ]]; then "$ROOT/scripts/uefi/build-esp.sh" "$ARCH" signed; fi

python3 "$ROOT/scripts/retain-build-files.py" "$STORAGE"
mkdir -p "$STORAGE"/{payloads,metadata,staging/system,staging/recovery,staging/data/Nova/Slots/A,staging/data/Nova/Slots/B,staging/data/Nova/Recovery,staging/data/Nova/Boot,staging/data/Nova/Update,staging/data/Users}
cp -a "$ROOTFS/System/." "$STORAGE/staging/system/"
cp -a "$ROOTFS/Recovery/." "$STORAGE/staging/recovery/"

SALT_A="$(printf 'NovaOS Stage-4D base|A|0.0.11' | sha256sum | awk '{print $1}')"
SALT_B="$(printf 'NovaOS Stage-4D base|B|0.0.11' | sha256sum | awk '{print $1}')"
"$ROOT/scripts/verity/build-system-slot.sh" \
  "$STORAGE/staging/system" "$STORAGE/payloads/system-a.slot" NOVA_SYSTEM_A \
  7c3d30aa-40c4-5de7-99c3-9897472a6f67 "$SALT_A" "$STORAGE/metadata/system-a.env"
"$ROOT/scripts/verity/build-system-slot.sh" \
  "$STORAGE/staging/system" "$STORAGE/payloads/system-b.slot" NOVA_SYSTEM_B \
  352d3363-8ab4-5c94-a93c-dc52a4d54a7a "$SALT_B" "$STORAGE/metadata/system-b.env"
cp "$RECOVERY_ARTIFACT/recovery.slot" "$STORAGE/payloads/recovery.slot"
cp "$RECOVERY_ARTIFACT/recovery.env" "$STORAGE/metadata/recovery.env"

if [[ -n "${NOVA_SIGNING_KEY:-}" ]]; then
  SIGNING_KEY="$NOVA_SIGNING_KEY"
else
  "$ROOT/scripts/ensure-dev-key.sh" >/dev/null
  SIGNING_KEY="$ROOT/build/dev-keys/NOVA-DEVELOPMENT-ONLY.key.pem"
fi
openssl x509 -inform DER -in "$ROOTFS/System/Trust/nova-root.der" -pubkey -noout > "$STORAGE/root-public-key.pem"

sign_descriptor() {
  local envfile="$1" outdir="$2" kind="$3" id="$4" version="$5" payload="$6" rollback_index="$7" comment="$8"
  # shellcheck disable=SC1090
  source "$envfile"
  cat > "$outdir/verity.manifest" <<MANIFEST
# NovaOS Stage-4D $comment
format=3
artifact_kind=$kind
update_id=$id
version=$version
rollback_index=$rollback_index
payload=$payload
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
  openssl dgst -sha256 -sign "$SIGNING_KEY" -out "$outdir/verity.sig" "$outdir/verity.manifest"
  openssl dgst -sha256 -verify "$STORAGE/root-public-key.pem" -signature "$outdir/verity.sig" "$outdir/verity.manifest" >/dev/null
}
sign_descriptor "$STORAGE/metadata/system-a.env" "$STORAGE/staging/data/Nova/Slots/A" system-slot base-stage4d-a 0.0.11-base system.slot 0 "signed base System A dm-verity descriptor"
sign_descriptor "$STORAGE/metadata/system-b.env" "$STORAGE/staging/data/Nova/Slots/B" system-slot base-stage4d-b 0.0.11-base system.slot 0 "signed base System B dm-verity descriptor"
# Keep a root-signed copy on Data for compatibility/diagnostics, but runtime
# Stage-4D trust prefers the firmware-bound /Trust copy and does not depend on
# Data availability for Recovery verification.
cp "$RECOVERY_ARTIFACT/verity.manifest" "$STORAGE/staging/data/Nova/Recovery/verity.manifest"
cp "$RECOVERY_ARTIFACT/verity.sig" "$STORAGE/staging/data/Nova/Recovery/verity.sig"
cmp -s "$ROOTFS/Trust/recovery-verity.manifest" "$RECOVERY_ARTIFACT/verity.manifest" || { echo "ERROR: initramfs Recovery descriptor does not match disk Recovery artifact" >&2; exit 2; }
cmp -s "$ROOTFS/Trust/recovery-verity.sig" "$RECOVERY_ARTIFACT/verity.sig" || { echo "ERROR: initramfs Recovery signature does not match disk Recovery artifact" >&2; exit 2; }

"$ROOT/scripts/build-update-bundle.sh" "$ARCH"
cp "$STORAGE/update-bundle/update.manifest" "$STORAGE/staging/data/Nova/Update/update.manifest"
cp "$STORAGE/update-bundle/update.sig" "$STORAGE/staging/data/Nova/Update/update.sig"
cp "$STORAGE/update-bundle/system.slot" "$STORAGE/staging/data/Nova/Update/system.slot"
cp "$STORAGE/update-bundle/release-policy.conf" "$STORAGE/staging/data/Nova/Update/release-policy.conf"
cp "$STORAGE/update-bundle/release-policy.sig" "$STORAGE/staging/data/Nova/Update/release-policy.sig"
cp "$STORAGE/update-bundle/release-active.der" "$STORAGE/staging/data/Nova/Update/release-active.der"
[[ ! -f "$STORAGE/update-bundle/release-next.der" ]] || cp "$STORAGE/update-bundle/release-next.der" "$STORAGE/staging/data/Nova/Update/release-next.der"

python3 - "$STORAGE/staging/data/Nova/Boot" <<'PYMETA'
import hashlib, sys
from pathlib import Path
out=Path(sys.argv[1]); out.mkdir(parents=True,exist_ok=True)
def render(seq):
    canonical=(f"format=3\nrecord_seq={seq}\nactive=A\nprevious=A\ngeneration=0\nconfirmed=yes\n"
               "attempts_left=0\npending_kind=none\nupdate_id=none\n")
    checksum=hashlib.sha256(canonical.encode()).hexdigest()
    return "# NovaOS Stage-4D redundant boot/update journal\n"+canonical+f"checksum={checksum}\n"
(out/'state.A').write_text(render(1),newline='\n')
(out/'state.B').write_text(render(2),newline='\n')
PYMETA
printf 'NovaOS persistent Data partition - Stage-4D\n' > "$STORAGE/staging/data/Nova/README.txt"

cp "$OUT/uefi/esp-signed.fat" "$STORAGE/payloads/esp.fat"

mkext4() {
  local label="$1" uuid="$2" mib="$3" source="$4" out="$5"
  truncate -s "${mib}M" "$out"
  E2FSPROGS_FAKE_TIME="${SOURCE_DATE_EPOCH:-0}" \
    mke2fs -q -t ext4 -F -L "$label" -U "$uuid" \
      -E lazy_itable_init=0,lazy_journal_init=0 -d "$source" "$out"
}
DATA_MIB=384
[[ "${NOVA_R47_BROWSER_COMPAT:-0}" == 1 ]] && DATA_MIB=1024
mkext4 NOVA_DATA 177e3ba3-10cb-5b35-acfc-36c91f774e7a "$DATA_MIB" "$STORAGE/staging/data" "$STORAGE/payloads/data.ext4"

python3 - "$STORAGE" <<'PY'
import json, sys
from pathlib import Path
s=Path(sys.argv[1]); M=1024*1024
spec={
  "format":"nova-stage4c-gpt-v1",
  "partitions":[
    {"name":"NOVA_ESP","type":"esp","size_bytes":(s/'payloads/esp.fat').stat().st_size,"payload":str(s/'payloads/esp.fat')},
    {"name":"NOVA_SYSTEM_A","type":"linux","size_bytes":(s/'payloads/system-a.slot').stat().st_size,"payload":str(s/'payloads/system-a.slot')},
    {"name":"NOVA_SYSTEM_B","type":"linux","size_bytes":(s/'payloads/system-b.slot').stat().st_size,"payload":str(s/'payloads/system-b.slot')},
    {"name":"NOVA_RECOVERY","type":"linux","size_bytes":24*M,"payload":str(s/'payloads/recovery.slot')},
    {"name":"NOVA_DATA","type":"linux","size_bytes":(s/'payloads/data.ext4').stat().st_size,"payload":str(s/'payloads/data.ext4')},
  ]
}
(s/'disk-spec.json').write_text(json.dumps(spec,indent=2)+'\n')
PY

python3 "$ROOT/scripts/storage/create-gpt-disk.py" build "$DISK" "$STORAGE/disk-spec.json"
python3 "$ROOT/scripts/storage/create-gpt-disk.py" inspect "$DISK"
sha256sum "$DISK" > "$DISK.sha256"
echo "Built Stage-4D storage image: $DISK"
echo "SHA-256: $(awk '{print $1}' "$DISK.sha256")"
