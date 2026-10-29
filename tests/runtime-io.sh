#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/test-runtime-io"
mkdir -p "$OUT"
clang --target=x86_64-linux-gnu \
  -ffreestanding -fno-builtin -fno-stack-protector -fno-pic \
  -nostdlib -static -Os -Wall -Wextra -Werror \
  -I"$ROOT/src/runtime" \
  "$ROOT/src/init/start.S" \
  "$ROOT/src/runtime/nova_runtime.c" \
  "$ROOT/tests/runtime-io.c" \
  -fuse-ld=lld -Wl,-e,_start -Wl,--build-id=none -Wl,-z,noexecstack \
  -o "$OUT/runtime-io"
"$OUT/runtime-io" | grep -Fqx 'Runtime I/O exact-string regression: PASS'
echo "Runtime I/O exact-string regression passed."
