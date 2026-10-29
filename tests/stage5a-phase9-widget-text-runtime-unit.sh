#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/runtime" \
  "$ROOT/src/runtime/nova_textedit.c" "$ROOT/src/runtime/nova_widget.c" \
  "$ROOT/tests/stage5a-phase9-widget-text-runtime-unit.c" -o "$TMP/widget-text-unit"
"$TMP/widget-text-unit"
