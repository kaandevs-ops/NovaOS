#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"; CFG="$ROOT/configs/kernel/common.config"; LAYOUT="$ROOT/configs/storage/layout.conf"
for n in '/dev/vda2' '/dev/vda3' '/dev/vda4' '/dev/vda5' '/dev/mapper/nova-system' '/dev/mapper/nova-recovery'; do grep -Fq "$n" "$SRC"; done
grep -Fq 'rw,nosuid,nodev,noexec' "$SRC"
grep -Fq 'nova_mount(NOVA_DM_NODE,"/System","ext4",NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV' "$SRC"
grep -Fq 'nova_mount(NOVA_RECOVERY_DM_NODE,"/Recovery","ext4",NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC' "$SRC"
grep -Fq 'data=96' "$LAYOUT"
for sym in CONFIG_EXT4_FS CONFIG_VIRTIO_BLK CONFIG_EFI_PARTITION CONFIG_BLK_DEV_DM CONFIG_DM_VERITY; do grep -qx "$sym=y" "$CFG"; done
echo "Stage-4B verified System + Recovery persistent storage source policy test passed."
