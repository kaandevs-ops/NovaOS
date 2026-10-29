#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src/runtime" "$ROOT/tests/r23-preferences.c" "$ROOT/src/runtime/nova_runtime.c" -o "$ROOT/build/x86_64/r17-native/test-r23-preferences"
"$ROOT/build/x86_64/r17-native/test-r23-preferences"
for arch in x86_64-linux-gnu aarch64-linux-gnu; do clang --target="$arch" -ffreestanding -Wall -Wextra -Werror -I"$ROOT/src/runtime" -fsyntax-only "$ROOT/tests/r23-preferences-syntax.c"; done
