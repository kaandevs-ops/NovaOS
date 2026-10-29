#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/r46 evidence/r46
clang -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -fsanitize=address,undefined -Isrc/runtime -Isrc/compat tests/r46-shm-import.c src/compat/nova_shm_import.c -pthread -o build/r46/test-shm-import
build/r46/test-shm-import | tee evidence/r46/shm-import-test.log
