#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-x86_64}"
case "$ARCH" in x86_64) TARGET=x86_64-linux-gnu;; arm64) TARGET=aarch64-linux-gnu;; *) exit 2;; esac
OUT="$ROOT/build/$ARCH/r16-integration"
mkdir -p "$OUT/System/Core" "$OUT/System/Apps"
COMMON=(--target="$TARGET" -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -ffunction-sections -fdata-sections -nostdlib -static -Os -Wall -Wextra -Werror -I"$ROOT/src/runtime")
RUNTIME=("$ROOT/src/init/start.S" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_sha256.c" "$ROOT/src/runtime/nova_tpm2.c" "$ROOT/src/runtime/nova_tcg2.c" "$ROOT/src/runtime/nova_font.c")
for item in init:src/init/main.c System/Core/nova-compositor:src/services/compositor.c sandbox-test:tests/r16-trusted-sandbox.c sandbox-launch:tests/r16-trusted-sandbox.c; do
 name="${item%%:*}"; source="${item#*:}"
 EXTRA=(); if [[ "$name" == sandbox-launch ]]; then EXTRA=(-DNOVA_R16_EXEC_FILES=1); fi
 clang "${COMMON[@]}" "${EXTRA[@]}" "${RUNTIME[@]}" "$ROOT/$source" -fuse-ld=lld -Wl,--gc-sections -Wl,-e,_start -Wl,-z,noexecstack -o "$OUT/$name"
done
bash "$ROOT/scripts/build-r16-files.sh" "$ARCH"
install -m 0755 "$ROOT/build/$ARCH/r16-files/nova-files" "$OUT/System/Apps/nova-files"
echo "R16_INIT_COMPOSITOR_FILES_LINK=PASS $TARGET"
