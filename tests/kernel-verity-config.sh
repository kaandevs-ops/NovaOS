#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CFG="$ROOT/configs/kernel/common.config"
grep -qx 'CONFIG_BLK_DEV_DM=y' "$CFG"
grep -qx 'CONFIG_DM_VERITY=y' "$CFG"
grep -Fq 'dm-verity runtime mapping    : ' "$ROOT/src/init/main.c"
! grep -Fq 'dm-verity runtime mapping    : NOT YET ENFORCED' "$ROOT/src/init/main.c"
echo "Stage-3C dm-verity runtime declaration passed."
