#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH=x86_64
"$ROOT/scripts/build-userland.sh" "$ARCH" >/dev/null
"$ROOT/scripts/build-initramfs.sh" "$ARCH" >/dev/null
ROOTFS="$ROOT/build/$ARCH/rootfs"
MANIFEST="$ROOTFS/System/Integrity/manifest.sha256"
[[ -s "$MANIFEST" ]]
count=0
while read -r hash guest; do
  [[ -z "${hash:-}" || "${hash:0:1}" == "#" ]] && continue
  host="$ROOTFS$guest"
  [[ -f "$host" ]]
  actual="$(sha256sum "$host" | awk '{print $1}')"
  [[ "$actual" == "$hash" ]]
  count=$((count+1))
done < "$MANIFEST"
[[ "$count" -ge 8 ]]
! grep -Fq '/System/Integrity/manifest.sig' "$MANIFEST"
! grep -Fq '/System/Integrity/manifest.sha256' "$MANIFEST"
echo "Integrity manifest verification passed ($count entries)."
