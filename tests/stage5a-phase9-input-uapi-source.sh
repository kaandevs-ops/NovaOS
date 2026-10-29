#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/src/runtime/nova_surface_protocol.h"; I="$ROOT/src/runtime/nova_input.h"
grep -Fq '#define NOVA_SURFACE_PROTOCOL_VERSION 6U' "$P"
grep -Fq 'NOVA_SURFACE_OP_SET_INPUT_FLAGS' "$P"
grep -Fq 'NOVA_SURFACE_INPUT_CAPTURE_TAB' "$P"
grep -Fq 'NOVA_SURFACE_INPUT_TEXT_ASCII' "$P"
grep -Fq 'unsigned int input_flags;' "$P"
grep -Fq 'sizeof(struct nova_surface_message) == 216' "$P"
for key in BACKSPACE TAB ENTER SPACE LEFT RIGHT HOME END DELETE A H I Z; do grep -Fq "NOVA_KEY_${key}" "$I"; done
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat >"$TMP/abi.c" <<'SRC'
#include "nova_surface_protocol.h"
#include "nova_input.h"
int phase9_abi(void) { return (int)(sizeof(struct nova_surface_message)+sizeof(struct nova_surface_event)+sizeof(struct nova_input_event)); }
SRC
for target in x86_64-linux-gnu aarch64-linux-gnu; do
  clang --target="$target" -ffreestanding -fno-builtin -nostdlib -Wall -Wextra -Werror -I"$ROOT/src/runtime" -c "$TMP/abi.c" -o "$TMP/${target}.o"
done
echo 'Stage-5A Phase-9 protocol v5/v6-compatible input-flags + keyboard UAPI ABI contract: PASS'
