#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/x86_64/r17-native"
BUILD_ID="$(python3 "$ROOT/tools/r24-build-id.py")"
clang "-DNOVA_BUILD_ID=\"$BUILD_ID\"" -std=c11 -O2 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 -I"$ROOT/src/runtime" -I"$ROOT/files/src" "$ROOT/tests/r17-real-compositor.c" "$ROOT/tests/r17-fixture-authority.c" \
 "$ROOT/files/src/nova_files_core.c" "$ROOT/files/src/nova_files_ui.c" "$ROOT/files/src/nova_files_font.c" \
 "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" -Wl,--gc-sections -o "$OUT/test-real-compositor"
