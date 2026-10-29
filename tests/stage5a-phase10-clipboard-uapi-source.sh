#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/src/runtime/nova_surface_protocol.h"; I="$ROOT/src/runtime/nova_input.h"
grep -Fq '#define NOVA_SURFACE_PROTOCOL_VERSION 6U' "$P"
grep -Fq '#define NOVA_SURFACE_CLIPBOARD_BYTES 64U' "$P"
for op in SET_CLIPBOARD GET_CLIPBOARD; do grep -Fq "NOVA_SURFACE_OP_${op}" "$P"; done
grep -Fq 'NOVA_SURFACE_EVENT_CLIPBOARD' "$P"
grep -Fq 'NOVA_SURFACE_INPUT_CLIPBOARD' "$P"
grep -Fq 'NOVA_INPUT_MOD_SHIFT' "$P"; grep -Fq 'NOVA_INPUT_MOD_CTRL' "$P"
for field in modifiers clipboard_length clipboard; do grep -Fq "$field" "$P"; done
grep -Fq 'sizeof(struct nova_surface_message) == 216' "$P"
grep -Fq 'sizeof(struct nova_surface_event) == 136' "$P"
for key in LEFTCTRL RIGHTCTRL LEFTSHIFT RIGHTSHIFT; do grep -Fq "NOVA_KEY_${key}" "$I"; done
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat >"$TMP/abi.c" <<'SRC'
#include "nova_surface_protocol.h"
#include "nova_input.h"
int phase10_abi(void) { return (int)(sizeof(struct nova_surface_message)+sizeof(struct nova_surface_event)+sizeof(struct nova_input_event)); }
SRC
for target in x86_64-linux-gnu aarch64-linux-gnu; do
  clang --target="$target" -ffreestanding -fno-builtin -nostdlib -Wall -Wextra -Werror -I"$ROOT/src/runtime" -c "$TMP/abi.c" -o "$TMP/${target}.o"
done
echo 'Stage-5A Phase-10 protocol v6 modifier + bounded clipboard UAPI ABI contract: PASS'
