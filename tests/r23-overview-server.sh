#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -fsanitize=address,undefined -I"$ROOT/src/runtime" "$ROOT/tests/r23-overview-server.c" "$ROOT/src/runtime/nova_runtime.c" -Wl,--gc-sections -o "$ROOT/build/x86_64/r17-native/test-r23-overview-server"
if [ "$(id -u)" -eq 0 ]; then
 "$ROOT/build/x86_64/r17-native/test-r23-overview-server"
else
 unshare -Ur "$ROOT/build/x86_64/r17-native/test-r23-overview-server"
fi
