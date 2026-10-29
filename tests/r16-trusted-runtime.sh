#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/x86_64/r16-integration"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 -fsanitize=address,undefined -I"$ROOT/src/runtime" "$ROOT/tests/r16-catalog-runtime.c" \
 "$ROOT/src/runtime/nova_runtime.c" -Wl,--gc-sections -o "$OUT/test-catalog"
(cd "$ROOT" && "$OUT/test-catalog")
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" \
 "$ROOT/tests/r16-grant-runtime.c" "$ROOT/src/runtime/nova_runtime.c" -o "$OUT/test-grant"
unshare -Ur "$OUT/test-grant"
