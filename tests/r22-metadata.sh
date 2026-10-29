#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" -I"$ROOT/files/src" "$ROOT/tests/r22-metadata.c" "$ROOT/files/src/nova_files_core.c" "$ROOT/files/src/nova_files_font.c" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" -o "$ROOT/build/x86_64/r17-native/test-r22-metadata"
"$ROOT/build/x86_64/r17-native/test-r22-metadata"
