#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/x86_64/r17-native"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -fsanitize=address,undefined \
 -I"$ROOT/src/runtime" "$ROOT/tests/r17-wm-metrics.c" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" \
 -Wl,--gc-sections -o "$OUT/test-wm-metrics"
"$OUT/test-wm-metrics"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" -I"$ROOT/files/src" \
 "$ROOT/tests/r17-files-ui.c" "$ROOT/files/src/nova_files_core.c" "$ROOT/files/src/nova_files_ui.c" "$ROOT/files/src/nova_files_font.c" \
 -o "$OUT/test-files-ui"
"$OUT/test-files-ui"

clang -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -fsanitize=address,undefined \
 -I"$ROOT/src/runtime" "$ROOT/tests/r17-buffer-safety.c" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" \
 -Wl,--gc-sections -o "$OUT/test-buffer-safety"
"$OUT/test-buffer-safety"
