#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/x86_64/r17-native"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" -I"$ROOT/files/src" \
 "$ROOT/tests/r19-native-design.c" "$ROOT/files/src/nova_files_core.c" "$ROOT/files/src/nova_files_ui.c" "$ROOT/files/src/nova_files_font.c" -o "$OUT/test-r19-native-design"
"$OUT/test-r19-native-design" "$ROOT/evidence/r21/R19_FONT_ICON_DIAGNOSTIC_HOST_NOT_VM.ppm"
