#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 9 ]]; then
  echo "usage: $0 <source-dir> <output-image> <label> <uuid> <salt-hex> <metadata-out> <data-mib> <slot-mib> <kind>" >&2
  exit 2
fi

SOURCE="$1"; OUT="$2"; LABEL="$3"; UUID="$4"; SALT="$5"; META="$6"
DATA_MIB="$7"; SLOT_MIB="$8"; KIND="$9"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MKE2FS_PROFILE="$ROOT/configs/storage/mke2fs-stage4d.conf"

command -v mke2fs >/dev/null 2>&1 || { echo "ERROR: mke2fs is required." >&2; exit 2; }
command -v debugfs >/dev/null 2>&1 || { echo "ERROR: debugfs is required for deterministic ext4 inode timestamp normalization." >&2; exit 2; }
command -v veritysetup >/dev/null 2>&1 || { echo "ERROR: veritysetup is required (cryptsetup-bin)." >&2; exit 2; }
[[ "$SALT" =~ ^[0-9a-fA-F]{64}$ ]] || { echo "ERROR: salt must be exactly 64 hex chars." >&2; exit 2; }
[[ -d "$SOURCE" ]] || { echo "ERROR: source directory not found: $SOURCE" >&2; exit 2; }
[[ -s "$MKE2FS_PROFILE" ]] || { echo "ERROR: deterministic mke2fs profile missing: $MKE2FS_PROFILE" >&2; exit 2; }
[[ "$DATA_MIB" =~ ^[0-9]+$ && "$SLOT_MIB" =~ ^[0-9]+$ ]] || { echo "ERROR: MiB sizes must be integers." >&2; exit 2; }
(( DATA_MIB > 0 && SLOT_MIB > DATA_MIB )) || { echo "ERROR: slot must be larger than data region." >&2; exit 2; }

DATA_BYTES=$((DATA_MIB*1024*1024))
SLOT_BYTES=$((SLOT_MIB*1024*1024))
DATA_BLOCK_SIZE=4096
HASH_BLOCK_SIZE=4096
(( DATA_BYTES % DATA_BLOCK_SIZE == 0 )) || { echo "ERROR: data size must align to 4096 bytes." >&2; exit 2; }
DATA_BLOCKS=$((DATA_BYTES / DATA_BLOCK_SIZE))
HASH_START_BLOCK=$DATA_BLOCKS
HASH_OFFSET=$DATA_BYTES

mkdir -p "$(dirname "$OUT")" "$(dirname "$META")"
python3 "$ROOT/scripts/retain-build-files.py" "$OUT" "$META" "$META.veritysetup"

# Reproducible ext4 input + metadata.  e2fsprogs 1.47.0 (Ubuntu 24.04)
# treats E2FSPROGS_FAKE_TIME=0 as "use wall clock", so keep the public
# SOURCE_DATE_EPOCH semantics while using a non-zero compatibility value for
# the older e2fsprogs fake-time interface.  The directory hash seed is also
# random unless explicitly fixed.
SOURCE_EPOCH="${SOURCE_DATE_EPOCH:-0}"
[[ "$SOURCE_EPOCH" =~ ^[0-9]+$ ]] || { echo "ERROR: SOURCE_DATE_EPOCH must be a non-negative integer." >&2; exit 2; }
E2FS_FAKE_TIME="$SOURCE_EPOCH"
[[ "$E2FS_FAKE_TIME" == 0 ]] && E2FS_FAKE_TIME=1
python3 - "$SOURCE" "$SOURCE_EPOCH" <<'PYMTIME'
import os, sys
root, epoch = sys.argv[1], int(sys.argv[2])
for base, dirs, files in os.walk(root):
    for name in dirs + files:
        path = os.path.join(base, name)
        try:
            # mke2fs 1.47.0 copies source atime/mtime/ctime into imported
            # inodes.  atime/mtime are settable; ctime is not and is
            # normalized inside the image after population below.
            os.utime(path, (epoch, epoch), follow_symlinks=False)
        except (NotImplementedError, OSError):
            pass
os.utime(root, (epoch, epoch))
PYMTIME

truncate -s "$DATA_BYTES" "$OUT"
# Do not inherit /etc/mke2fs.conf.  e2fsprogs 1.47.0 changed the default ext4
# feature set (notably metadata_csum_seed and orphan_file), so host defaults
# are not a stable on-disk format contract.  NovaOS supplies an explicit
# profile and repeats the security/reproducibility-sensitive feature choices
# on the command line.  metadata_csum remains enabled, while the two newer
# host-default features are disabled until NovaOS deliberately versions them.
MKE2FS_CONFIG="$MKE2FS_PROFILE" \
SOURCE_DATE_EPOCH="$SOURCE_EPOCH" E2FSPROGS_FAKE_TIME="$E2FS_FAKE_TIME" \
  mke2fs -q -t ext4 -F -b 4096 -I 256 -m 0 -L "$LABEL" -U "$UUID" \
    -O "has_journal,extent,huge_file,flex_bg,metadata_csum,64bit,dir_nlink,extra_isize,^metadata_csum_seed,^orphan_file" \
    -E "nodiscard,lazy_itable_init=0,lazy_journal_init=0,hash_seed=$UUID" \
    -d "$SOURCE" "$OUT"

# e2fsprogs 1.47.0 has a reproducibility trap that SOURCE_DATE_EPOCH alone
# cannot fix: mke2fs -d calls set_inode_extra(), which copies the host
# source st_ctime verbatim into each imported ext4 inode.  POSIX does not
# allow userspace to set ctime, and the os.utime() normalization above
# actually advances host ctime on every clean build.  e2fsprogs 1.47.1
# fixed this by clamping imported timestamps to fs->now when fake time is
# active.  Ubuntu 24.04 ships 1.47.0, so Stage-4D backports exactly the
# missing semantic after population: normalize ctime of every imported
# path to E2FS_FAKE_TIME using debugfs, which rewrites inode checksums.
# atime/mtime remain SOURCE_DATE_EPOCH; crtime is already generated from
# fs->now by ext2fs_write_new_inode().
DEBUGFS_CMDS="$(mktemp)"
trap 'python3 "$ROOT/scripts/retain-build-files.py" "$DEBUGFS_CMDS"' EXIT
python3 - "$SOURCE" "$E2FS_FAKE_TIME" "$DEBUGFS_CMDS" <<'PYDEBUGFS'
import os, sys
source, epoch, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
paths = []
for base, dirs, files in os.walk(source, topdown=True, followlinks=False):
    dirs.sort(); files.sort()
    for name in dirs + files:
        host = os.path.join(base, name)
        rel = os.path.relpath(host, source).replace(os.sep, '/')
        if any(ch in rel for ch in ('\n', '\r', '\x00')):
            raise SystemExit(f'unsupported control character in ext4 source path: {rel!r}')
        guest = '/' + rel
        # debugfs uses libss argument parsing; quote and escape path safely.
        guest = guest.replace('\\', '\\\\').replace('"', '\\"')
        paths.append(guest)
with open(out, 'w', encoding='utf-8', newline='\n') as f:
    f.write(f'set_current_time {epoch}\n')
    for guest in paths:
        f.write(f'set_inode_field "{guest}" ctime @{epoch}\n')
PYDEBUGFS
if ! debugfs -w -f "$DEBUGFS_CMDS" "$OUT" >/dev/null 2>&1; then
  echo "ERROR: debugfs failed while normalizing imported ext4 ctime fields." >&2
  exit 1
fi
python3 "$ROOT/scripts/retain-build-files.py" "$DEBUGFS_CMDS"
trap - EXIT

truncate -s "$SLOT_BYTES" "$OUT"

veritysetup format \
  --no-superblock \
  --data-block-size="$DATA_BLOCK_SIZE" \
  --hash-block-size="$HASH_BLOCK_SIZE" \
  --data-blocks="$DATA_BLOCKS" \
  --hash-offset="$HASH_OFFSET" \
  --salt="$SALT" \
  "$OUT" "$OUT" | tee "$META.veritysetup" >/dev/null

ROOT_HASH="$(awk -F: 'tolower($1) ~ /root hash/ {gsub(/[[:space:]]/,"",$2); print tolower($2); exit}' "$META.veritysetup")"
ACTUAL_SALT="$(awk -F: 'tolower($1) ~ /^salt/ {gsub(/[[:space:]]/,"",$2); print tolower($2); exit}' "$META.veritysetup")"
[[ "$ROOT_HASH" =~ ^[0-9a-f]{64}$ ]] || { echo "ERROR: could not parse dm-verity root hash." >&2; cat "$META.veritysetup" >&2; exit 1; }
[[ "$ACTUAL_SALT" == "${SALT,,}" ]] || { echo "ERROR: veritysetup salt mismatch." >&2; exit 1; }

veritysetup verify \
  --no-superblock \
  --data-block-size="$DATA_BLOCK_SIZE" \
  --hash-block-size="$HASH_BLOCK_SIZE" \
  --data-blocks="$DATA_BLOCKS" \
  --hash-offset="$HASH_OFFSET" \
  --salt="$ACTUAL_SALT" \
  "$OUT" "$OUT" "$ROOT_HASH" >/dev/null

PAYLOAD_SHA256="$(sha256sum "$OUT" | awk '{print $1}')"
PAYLOAD_SIZE="$(stat -c %s "$OUT")"
[[ "$PAYLOAD_SIZE" -eq "$SLOT_BYTES" ]] || { echo "ERROR: image size mismatch." >&2; exit 1; }

cat > "$META" <<META
payload_sha256=$PAYLOAD_SHA256
payload_size=$PAYLOAD_SIZE
verity_data_block_size=$DATA_BLOCK_SIZE
verity_hash_block_size=$HASH_BLOCK_SIZE
verity_data_blocks=$DATA_BLOCKS
verity_hash_start_block=$HASH_START_BLOCK
verity_hash_algorithm=sha256
verity_root_hash=$ROOT_HASH
verity_salt=$ACTUAL_SALT
META

python3 "$ROOT/scripts/retain-build-files.py" "$META.veritysetup"
printf 'Built verity %s image: %s\n' "$KIND" "$OUT"
printf '  root_hash=%s data_blocks=%s hash_start=%s\n' "$ROOT_HASH" "$DATA_BLOCKS" "$HASH_START_BLOCK"
