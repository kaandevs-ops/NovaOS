#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/tcg2-parser-host"
mkdir -p "$(dirname "$OUT")"
cc -std=c11 -Wall -Wextra -Werror -DNOVA_TCG2_PARSER_ONLY -ffunction-sections -fdata-sections -Wl,--gc-sections -I"$ROOT/src/runtime" \
  "$ROOT/tests/tcg2-parser-host.c" "$ROOT/src/runtime/nova_tcg2.c" "$ROOT/src/runtime/nova_sha256.c" -o "$OUT"
"$OUT"
SRC="$ROOT/src/init/main.c"
grep -Fq 'NOVA_TCG_EVENT_LOG "/sys/kernel/security/tpm0/binary_bios_measurements"' "$SRC"
grep -Fq 'nova_tcg2_collect_evidence(NOVA_TCG_EVENT_LOG,&evidence)' "$SRC"
grep -Fq 'nova_streq(g_event_log_replayed_pcr7,g_pcr7_sha256)' "$SRC"
grep -Fq 'try_mount("securityfs", "/sys/kernel/security", "securityfs"' "$SRC"
grep -Fq 'nova_fd_puts((int)fd,"event_log_replay=")' "$SRC"
grep -Fq 'g_event_log_verified?"VERIFIED":"REJECTED"' "$SRC"
grep -Fq 'g_tcg2_log[TCG2_EVENT_LOG_MAX + 1U]' "$ROOT/src/runtime/nova_tcg2.c"
grep -Fq 'out->log_bytes=total' "$ROOT/src/runtime/nova_tcg2.c"
grep -Fq 'nova_sha256_hex(log_digest,out->log_sha256)' "$ROOT/src/runtime/nova_tcg2.c"
