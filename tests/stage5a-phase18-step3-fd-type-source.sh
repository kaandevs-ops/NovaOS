#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
B="$ROOT/src/services/resource_broker.c"
H="$ROOT/src/runtime/nova_linux.h"
for marker in 'NOVA_NR_statx' 'NOVA_AT_EMPTY_PATH' 'NOVA_STATX_TYPE' 'NOVA_STATX_SIZE' 'NOVA_S_IFREG' 'struct nova_statx' 'stx_size' ; do
  grep -Fq "$marker" "$H" || { echo "missing UAPI marker: $marker" >&2; exit 1; }
done
for marker in 'grant_fd_regular_bounded((int)resource_fd)' 'nova_syscall5(NOVA_NR_statx,fd,(long)""' 'NOVA_RESOURCE_DEMO_MAX_BYTES 65536ULL' 'NOVA_RESOURCE_BROKER_RESULT_INTERNAL' 'NOVA_RESOURCE_BROKER_OP_GRANT_FD' ; do
  grep -Fq "$marker" "$B" || { echo "missing broker marker: $marker" >&2; exit 1; }
done
python3 "$ROOT/tests/stage5a-phase18-step3-statx-model.py"
CC="${CC:-clang}"
for TARGET in x86_64-linux-gnu aarch64-linux-gnu; do
  "$CC" --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only -I"$ROOT/src/runtime" "$B"
done
echo 'Phase-18 Step-3 R1 broker file-type/size source + dual-arch syntax: PASS'
