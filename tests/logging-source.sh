#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"
grep -Fq 'log_simple("INFO","trust","signature_verified","signed integrity manifest verified by pinned NovaOS root");' "$SRC"
grep -Fq 'nova_fd_write_all(NOVA_STDOUT,buffer,(nova_size_t)n)' "$SRC"
! grep -Fq 'pinned NovaOS rot' "$SRC"
echo "Structured logging source regression passed."
