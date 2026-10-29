#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH=x86_64
"$ROOT/scripts/build-userland.sh" "$ARCH" >/dev/null
"$ROOT/scripts/build-initramfs.sh" "$ARCH" >/dev/null
ROOTFS="$ROOT/build/$ARCH/rootfs"
CERT="$ROOTFS/System/Trust/nova-root.der"
PIN="$(sha256sum "$CERT" | awk '{print $1}')"
grep -aq "$PIN" "$ROOTFS/init"
grep -Fqx "cert_sha256=$PIN" "$ROOTFS/System/Trust/trust.conf"
echo "Root trust pin verification passed: $PIN"
