#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
for target in x86_64-linux-gnu aarch64-linux-gnu; do
 for source in "$ROOT/src/init/main.c" "$ROOT/src/services/"*.c; do
  clang --target="$target" -std=c11 -ffreestanding -fno-builtin -Wall -Wextra -Werror \
   -fsyntax-only -I"$ROOT/src/runtime" "$source"
 done
 echo "R16_EXISTING_USERLAND_SYNTAX=PASS $target"
done
