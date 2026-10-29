#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-x86_64}"
case "$ARCH" in x86_64) TARGET=x86_64-linux-gnu;; arm64) TARGET=aarch64-linux-gnu;; *) exit 2;; esac
OUT="$ROOT/build/$ARCH/r16-files"
mkdir -p "$OUT"
clang --target="$TARGET" -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic \
 -nostdlib -static -Os -ffunction-sections -fdata-sections -Wall -Wextra -Werror -I"$ROOT/src/runtime" -I"$ROOT/files/src" \
 "$ROOT/src/init/start.S" "$ROOT/src/runtime/nova_runtime.c" \
 "$ROOT/files/src/nova_files_memory.c" "$ROOT/files/src/nova_files_app.c" "$ROOT/files/src/nova_files_core.c" \
 "$ROOT/files/src/nova_files_ui.c" "$ROOT/files/src/nova_files_font.c" \
 "$ROOT/files/src/nova_files_surface_bridge.c" -fuse-ld=lld \
 -Wl,-e,_start -Wl,--gc-sections -Wl,--build-id=none -Wl,-z,noexecstack -o "$OUT/nova-files"
echo "R16_FILES_BUILD_PASS=$TARGET $OUT/nova-files"
