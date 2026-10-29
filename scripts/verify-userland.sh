#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

for arch in arm64 x86_64; do
  "$ROOT/scripts/build-userland.sh" "$arch"
  echo "== $arch =="
  for rel in init System/Core/nova-heartbeat; do
    bin="$ROOT/build/$arch/rootfs/$rel"
    file "$bin"
    if command -v readelf >/dev/null 2>&1; then
      readelf -h "$bin" | grep -E 'Class:|Machine:|Type:'
      if readelf -d "$bin" 2>/dev/null | grep -q NEEDED; then
        echo "ERROR: dynamic dependency found in $bin" >&2
        exit 1
      fi
    fi
  done
  "$ROOT/scripts/build-initramfs.sh" "$arch"
done

echo "Userland verification passed for arm64 and x86_64."
