#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
required=( '"verity="' '"verity_mapping_active="' '"verity_descriptor_verified="' '"verity_rollback_scheduled="' '"verity_root_hash="' '"recovery_verity="' '"recovery_verity_mapping_active="' '"recovery_verity_descriptor_verified="' '"recovery_verity_root_hash="' )
for needle in "${required[@]}"; do grep -Fq "$needle" "$SRC" || { echo "missing boot.state verity field: $needle" >&2; exit 1; }; done
grep -Fq 'nova_fd_putln((int)fd,verity_name())' "$SRC"
grep -Fq 'nova_fd_putln((int)fd,recovery_verity_name())' "$SRC"
echo "Stage-4A machine-readable System + Recovery verity state coverage passed."
