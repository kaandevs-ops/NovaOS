#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"; CFG="$ROOT/configs/kernel/common.config"; LAYOUT="$ROOT/configs/storage/layout.conf"
for n in '/dev/vda2' '/dev/vda3' '/dev/vda4' '/dev/vda5'; do grep -Fq "$n" "$SRC"; done
grep -Fq 'rw,nosuid,nodev,noexec' "$SRC"
grep -Fq 'data=96' "$LAYOUT"
for sym in CONFIG_EXT4_FS CONFIG_VIRTIO_BLK CONFIG_EFI_PARTITION; do grep -qx "$sym=y" "$CFG"; done
echo "Stage-3B persistent storage source policy test passed."
