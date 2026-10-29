#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" -I"$ROOT/files/src" \
 "$ROOT/tests/r18-visual-ui.c" "$ROOT/files/src/nova_files_core.c" "$ROOT/files/src/nova_files_ui.c" \
 "$ROOT/files/src/nova_files_font.c" "$ROOT/files/src/nova_files_surface_bridge.c" -o "$ROOT/build/x86_64/r17-native/test-r18-visual-ui"
"$ROOT/build/x86_64/r17-native/test-r18-visual-ui"
