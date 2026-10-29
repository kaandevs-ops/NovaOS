#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; CFG="$ROOT/configs/kernel/common.config"; SRC="$ROOT/src/init/main.c"
for symbol in CONFIG_BLK_DEV_DM CONFIG_DM_VERITY CONFIG_CRYPTO_SHA256 CONFIG_EXT4_FS CONFIG_DEVTMPFS; do grep -qx "${symbol}=y" "$CFG"; done
grep -Fq 'System dm-verity mapping     : ' "$SRC"
grep -Fq 'Recovery dm-verity mapping   : ' "$SRC"
grep -Fq 'ENFORCED' "$SRC"
echo "Stage-4A dm-verity runtime kernel configuration test passed."
