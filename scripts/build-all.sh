#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-arm64}"
if [[ "$(uname -s)" != Linux ]]; then
  echo "Full kernel/disk build currently requires a Linux build host/VM." >&2
  echo "Userland itself can be cross-compiled anywhere with LLVM+LLD." >&2
  exit 2
fi
"$ROOT/scripts/build-userland.sh" "$ARCH"
"$ROOT/scripts/build-initramfs.sh" "$ARCH"
"$ROOT/scripts/build-kernel.sh" "$ARCH"
"$ROOT/scripts/uefi/build-esp.sh" "$ARCH" signed
"$ROOT/scripts/build-disk.sh" "$ARCH"
echo "NovaOS Stage-5A Phase-1 build complete for $ARCH."
