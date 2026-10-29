#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"
LINUX="$ROOT/src/runtime/nova_linux.h"
SYSCALL="$ROOT/src/runtime/nova_syscall.h"
for needle in \
  'static void unmount_persistent_filesystems(void)' \
  'nova_umount2("/Recovery",0)' \
  'nova_umount2("/System",0)' \
  'nova_umount2("/Data",0)' \
  'shutdown_services();' \
  'write_boot_state();' \
  'unmount_persistent_filesystems();'; do grep -Fq "$needle" "$SRC"; done
grep -Fq 'NOVA_NR_umount2' "$LINUX"
grep -Fq 'static inline long nova_umount2' "$SYSCALL"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
block=s[s.index('static void ordered_shutdown'):s.index('static int execute_command')]
assert block.index('shutdown_services();') < block.index('write_boot_state();') < block.index('unmount_persistent_filesystems();') < block.index('nova_reboot(reboot_command);')
PY
echo "Stage-4B clean filesystem shutdown source test passed."
