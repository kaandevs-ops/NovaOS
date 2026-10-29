#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -fsanitize=address,undefined \
 -I"$ROOT/src/runtime" "$ROOT/tests/r19-compositor-design.c" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" \
 -Wl,--gc-sections -o "$ROOT/build/x86_64/r17-native/test-r19-compositor-design"
"$ROOT/build/x86_64/r17-native/test-r19-compositor-design"
