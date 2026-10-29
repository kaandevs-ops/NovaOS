#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/r24-units";mkdir -p "$OUT"
for test in input-damage diagnostics state-buffer; do clang -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -fsanitize=address,undefined -I"$ROOT/src/runtime" "$ROOT/tests/r24-$test.c" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" -Wl,--gc-sections -o "$OUT/$test";"$OUT/$test";done
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" -I"$ROOT/files/src" "$ROOT/tests/r24-properties.c" "$ROOT/files/src/nova_files_core.c" "$ROOT/files/src/nova_files_ui.c" "$ROOT/files/src/nova_files_font.c" -o "$OUT/properties";"$OUT/properties"

clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" -I"$ROOT/files/src" "$ROOT/tests/r24-name-filter.c" "$ROOT/files/src/nova_files_core.c" "$ROOT/files/src/nova_files_ui.c" "$ROOT/files/src/nova_files_surface_bridge.c" "$ROOT/files/src/nova_files_font.c" -o "$OUT/name-filter";"$OUT/name-filter"

clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" "$ROOT/tests/r24-wallpaper-cache.c" -o "$OUT/wallpaper-cache";"$OUT/wallpaper-cache"

clang -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -fsanitize=address,undefined -I"$ROOT/src/runtime" "$ROOT/tests/r24-renderer-ui.c" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" -Wl,--gc-sections -o "$OUT/renderer-ui";"$OUT/renderer-ui"
