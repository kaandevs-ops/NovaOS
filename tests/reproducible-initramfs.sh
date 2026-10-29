#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH=x86_64
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

build_fresh() {
  local snapshot="$1"
  rm -rf "$ROOT/build/$ARCH"
  "$ROOT/scripts/build-userland.sh" "$ARCH" >/dev/null
  "$ROOT/scripts/build-initramfs.sh" "$ARCH" >/dev/null
  mkdir -p "$snapshot"
  cp "$ROOT/build/$ARCH/initramfs.cpio.gz" "$snapshot/initramfs.cpio.gz"
  cp "$ROOT/build/$ARCH/recovery-artifact/recovery.slot" "$snapshot/recovery.slot"
  cp "$ROOT/build/$ARCH/recovery-artifact/verity.manifest" "$snapshot/verity.manifest"
  printf '%s %s %s\n' \
    "$(sha256sum "$snapshot/initramfs.cpio.gz" | awk '{print $1}')" \
    "$(sha256sum "$snapshot/recovery.slot" | awk '{print $1}')" \
    "$(sha256sum "$snapshot/verity.manifest" | awk '{print $1}')"
}

one="$(build_fresh "$TMP/first")"
two="$(build_fresh "$TMP/second")"
if [[ "$one" != "$two" ]]; then
  echo "ERROR: clean-build initramfs/Recovery artifacts are not reproducible." >&2
  echo "first : $one" >&2
  echo "second: $two" >&2
  echo "first differing Recovery image byte offsets (cmp -l, up to 16):" >&2
  cmp -l "$TMP/first/recovery.slot" "$TMP/second/recovery.slot" 2>/dev/null | head -n 16 >&2 || true
  if command -v dumpe2fs >/dev/null 2>&1; then
    dumpe2fs -h "$TMP/first/recovery.slot" 2>/dev/null > "$TMP/first-super.txt" || true
    dumpe2fs -h "$TMP/second/recovery.slot" 2>/dev/null > "$TMP/second-super.txt" || true
    echo "ext4 superblock header diff:" >&2
    diff -u "$TMP/first-super.txt" "$TMP/second-super.txt" >&2 || true
  fi
  if command -v debugfs >/dev/null 2>&1; then
    for guest in /README.txt /recovery.conf; do
      echo "ext4 inode stat diff for $guest:" >&2
      debugfs -R "stat $guest" "$TMP/first/recovery.slot" 2>/dev/null > "$TMP/first-inode.txt" || true
      debugfs -R "stat $guest" "$TMP/second/recovery.slot" 2>/dev/null > "$TMP/second-inode.txt" || true
      diff -u "$TMP/first-inode.txt" "$TMP/second-inode.txt" >&2 || true
    done
  fi
  exit 1
fi
read -r initramfs_sha recovery_sha manifest_sha <<< "$one"
echo "Reproducible clean-build initramfs passed: initramfs=$initramfs_sha recovery=$recovery_sha descriptor=$manifest_sha"
