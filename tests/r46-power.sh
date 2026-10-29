#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/r46 evidence/r46
TEST_DIR=$(mktemp -d "$PWD/evidence/r46/power-XXXXXXXX")
for part in backend ui; do
 clang -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-misleading-indentation -ffunction-sections -fdata-sections -fsanitize=address,undefined -DNOVA_POWER_PATH=\"$TEST_DIR/power.sock\" -Isrc/runtime "tests/r46-power-$part.c" src/runtime/nova_runtime.c src/runtime/nova_font.c -Wl,--gc-sections -o "$TEST_DIR/$part"
 "$TEST_DIR/$part" | tee "$TEST_DIR/$part.log"
done
