#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OLD="$(dirname "$ROOT")/phase19-desktop-r17-evolution-candidate"
clang -std=c11 -O2 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I"$OLD/src/runtime" -I"$OLD/files/src" \
 "$ROOT/tests/r18-r17-reference-compositor.c" "$OLD/tests/r17-fixture-authority.c" \
 "$OLD/files/src/nova_files_core.c" "$OLD/files/src/nova_files_ui.c" "$OLD/files/src/nova_files_font.c" \
 "$OLD/src/runtime/nova_runtime.c" "$OLD/src/runtime/nova_font.c" -Wl,--gc-sections -o "$ROOT/build/x86_64/r17-native/test-r17-reference"
