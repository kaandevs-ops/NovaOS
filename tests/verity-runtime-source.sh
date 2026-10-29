#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"; LINUX="$ROOT/src/runtime/nova_linux.h"; SYS="$ROOT/src/runtime/nova_syscall.h"
for needle in \
  'NOVA_DM_CONTROL "/dev/mapper/control"' \
  'NOVA_DM_NODE "/dev/mapper/nova-system"' \
  'NOVA_RECOVERY_DM_NODE "/dev/mapper/nova-recovery"' \
  'setup_verity_mapping_named' \
  'target_type,sizeof(spec->target_type),"verity"' \
  'NOVA_DM_TABLE_LOAD' 'NOVA_DM_DEV_SUSPEND' \
  'nova_mount(NOVA_DM_NODE,"/System","ext4"' \
  'nova_mount(NOVA_RECOVERY_DM_NODE,"/Recovery","ext4"'; do grep -Fq "$needle" "$SRC"; done
for needle in 'struct nova_dm_ioctl' 'struct nova_dm_target_spec' 'NOVA_DM_DEV_CREATE' 'NOVA_DM_TABLE_LOAD'; do grep -Fq "$needle" "$LINUX"; done
for needle in 'nova_ioctl' 'nova_mknod' 'nova_makedev'; do grep -Fq "$needle" "$SYS"; done
echo "Stage-4A runtime System + Recovery dm-verity source policy test passed."
