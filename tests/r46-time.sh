#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/x86_64/r17-native evidence/r46
clang -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -ffunction-sections -fdata-sections -fsanitize=address,undefined -Isrc/runtime tests/r46-time.c src/runtime/nova_runtime.c src/runtime/nova_font.c -Wl,--gc-sections -o build/x86_64/r17-native/test-r46-time
build/x86_64/r17-native/test-r46-time evidence/r46 | tee evidence/r46/time-test.log
