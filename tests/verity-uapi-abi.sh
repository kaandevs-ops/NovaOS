#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
H="$ROOT/src/runtime/nova_linux.h"
for needle in \
  '_Static_assert(sizeof(struct nova_dm_ioctl) == 312' \
  '_Static_assert(sizeof(struct nova_dm_target_spec) == 40' \
  '#define NOVA_NR_ioctl' '#define NOVA_NR_mknodat'; do grep -Fq "$needle" "$H"; done
echo "Stage-4A device-mapper UAPI ABI guard test passed."
