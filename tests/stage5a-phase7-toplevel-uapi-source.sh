#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/src/runtime/nova_surface_protocol.h"
grep -Eq '#define NOVA_SURFACE_PROTOCOL_VERSION [456]U' "$P"
grep -Fq '#define NOVA_SURFACE_TITLE_BYTES 32U' "$P"
for op in SET_ROLE SET_TITLE SET_SIZE_LIMITS ACK_CONFIGURE DESTROY_SURFACE; do grep -Fq "NOVA_SURFACE_OP_${op}" "$P"; done
for ev in CONFIGURE CLOSE_REQUEST; do grep -Fq "NOVA_SURFACE_EVENT_${ev}" "$P"; done
grep -Fq 'NOVA_SURFACE_ROLE_TOPLEVEL' "$P"
for field in role configure_serial min_width min_height max_width max_height title; do grep -Fq "$field" "$P"; done
grep -Eq 'sizeof\(struct nova_surface_message\) == (144|152|216)' "$P"
grep -Fq 'sizeof(struct nova_surface_event) == 136' "$P"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat >"$TMP/abi.c" <<'SRC'
#include "nova_surface_protocol.h"
int phase7_abi(void) { return (int)(sizeof(struct nova_surface_message)+sizeof(struct nova_surface_event)); }
SRC
for target in x86_64-linux-gnu aarch64-linux-gnu; do
  clang --target="$target" -ffreestanding -fno-builtin -nostdlib -Wall -Wextra -Werror -I"$ROOT/src/runtime" -c "$TMP/abi.c" -o "$TMP/${target}.o"
done
echo 'Stage-5A Phase-7 toplevel/configure/close UAPI compatibility contract: PASS'
