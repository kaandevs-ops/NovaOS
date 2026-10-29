#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/x86_64/r16-files"
for test in host parser transaction; do
 SRC=("$ROOT/files/src/nova_files_core.c")
 if [[ "$test" == host ]]; then SRC+=("$ROOT/files/src/nova_files_linux_host.c"); fi
 clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -I"$ROOT/files/src" "${SRC[@]}" "$ROOT/files/tests/test_files_$test.c" -o "$OUT/core-$test"
 "$OUT/core-$test"
done
