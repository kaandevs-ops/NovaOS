#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; G="$ROOT/scripts/verity/build-verity-volume.sh"; S="$ROOT/scripts/verity/build-system-slot.sh"; R="$ROOT/scripts/verity/build-recovery-slot.sh"; P="$ROOT/scripts/verity/prepare-recovery-artifact.sh"; D="$ROOT/scripts/build-disk.sh"
for needle in 'veritysetup format' '--no-superblock' '--data-block-size=' '--hash-block-size=' '--data-blocks=' '--hash-offset=' 'veritysetup verify' 'verity_root_hash='; do grep -Fq -- "$needle" "$G"; done
grep -Fq '40 48 System' "$S"; grep -Fq '20 24 Recovery' "$R"
for needle in 'system-a.slot' 'system-b.slot' 'artifact_kind=$kind' 'openssl dgst -sha256 -sign' 'openssl dgst -sha256 -verify'; do grep -Fq "$needle" "$D"; done
for needle in 'recovery.slot' 'artifact_kind=recovery-slot' 'openssl dgst -sha256 -sign' 'openssl dgst -sha256 -verify'; do grep -Fq "$needle" "$P"; done
for needle in 'MKE2FS_CONFIG="$MKE2FS_PROFILE"' 'mke2fs-stage4d.conf' '-b 4096' '-I 256' '-m 0' '^metadata_csum_seed' '^orphan_file' 'nodiscard' 'hash_seed=$UUID' 'E2FS_FAKE_TIME' '[[ "$E2FS_FAKE_TIME" == 0 ]] && E2FS_FAKE_TIME=1' 'os.utime(path, (epoch, epoch)' 'command -v debugfs' 'set_inode_field "{guest}" ctime @{epoch}' 'debugfs -w -f "$DEBUGFS_CMDS"' ; do
  grep -Fq -- "$needle" "$ROOT/scripts/verity/build-verity-volume.sh" || { echo "missing reproducible ext4 contract: $needle" >&2; exit 1; }
done
PROFILE="$ROOT/configs/storage/mke2fs-stage4d.conf"
[[ -s "$PROFILE" ]] || { echo "missing NovaOS deterministic mke2fs profile" >&2; exit 1; }
for needle in 'enable_periodic_fsck = 0' 'blocksize = 4096' 'inode_size = 256' 'reserved_ratio = 0' 'metadata_csum' 'small = {'; do
  grep -Fq -- "$needle" "$PROFILE" || { echo "missing deterministic mke2fs profile contract: $needle" >&2; exit 1; }
done
! grep -E '^[[:space:]]*features[[:space:]]*=.*metadata_csum_seed' "$PROFILE" >/dev/null || { echo "metadata_csum_seed must not be enabled by the NovaOS profile" >&2; exit 1; }
! grep -E '^[[:space:]]*features[[:space:]]*=.*orphan_file' "$PROFILE" >/dev/null || { echo "orphan_file must not be enabled by the NovaOS profile" >&2; exit 1; }
echo "Stage-4D host-independent deterministic ext4 + e2fsprogs-1.47.0 ctime normalization + generic dm-verity build policy test passed."
