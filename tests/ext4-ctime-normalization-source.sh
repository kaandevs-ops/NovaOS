#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
G="$ROOT/scripts/verity/build-verity-volume.sh"
for needle in \
  'e2fsprogs 1.47.0 has a reproducibility trap' \
  'set_inode_field "{guest}" ctime @{epoch}' \
  'debugfs -w -f "$DEBUGFS_CMDS"' \
  'atime/mtime remain SOURCE_DATE_EPOCH' \
  'crtime is already generated from'
do
  grep -Fq -- "$needle" "$G" || { echo "missing ext4 ctime normalization contract: $needle" >&2; exit 1; }
done
# Guard against regression to the incorrect assumption that os.utime can
# normalize ctime on the host.  It may set atime/mtime, but ctime must be
# normalized inside the ext4 image for e2fsprogs 1.47.0.
grep -Fq 'os.utime(path, (epoch, epoch)' "$G"
grep -Fq 'ctime is not' "$G"
echo "Stage-4D e2fsprogs-1.47.0 imported-inode ctime normalization source contract passed."
