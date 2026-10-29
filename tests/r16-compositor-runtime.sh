#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/x86_64/r16-integration/test-compositor"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 -fsanitize=address,undefined -I"$ROOT/src/runtime" "$ROOT/tests/r16-compositor-runtime.c" \
 "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" -Wl,--gc-sections -o "$OUT"
"$OUT"
