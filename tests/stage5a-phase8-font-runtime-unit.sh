#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/runtime" \
  "$ROOT/src/runtime/nova_font.c" "$ROOT/tests/stage5a-phase8-font-runtime-unit.c" -o "$TMP/font-unit"
"$TMP/font-unit"
