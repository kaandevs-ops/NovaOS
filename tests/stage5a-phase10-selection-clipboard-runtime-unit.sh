#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/runtime" \
  "$ROOT/src/runtime/nova_textedit.c" "$ROOT/tests/stage5a-phase10-selection-clipboard-runtime-unit.c" \
  -o "$TMP/phase10-textedit-unit"
"$TMP/phase10-textedit-unit"
