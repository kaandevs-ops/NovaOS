#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HDR="$ROOT/src/runtime/nova_app_metadata_protocol.h"
BASE="$ROOT/src/runtime/nova_app_catalog_protocol.h"

[[ -f "$HDR" ]] || { echo 'FAIL: nova_app_metadata_protocol.h missing' >&2; exit 1; }

grep -q '#define NOVA_APP_CATALOG_PROTOCOL_VERSION 1U' "$BASE"
grep -q '#define NOVA_APP_CATALOG_OP_GET_METADATA 3U' "$HDR"
grep -q '#define NOVA_APP_CATALOG_OP_METADATA 0x104U' "$HDR"
grep -q '#define NOVA_APP_DESCRIPTION_BYTES 128U' "$HDR"
grep -q '#define NOVA_APP_ICON_ASSET_ID_BYTES 48U' "$HDR"
grep -q 'NOVA_APP_CATEGORY_DEVELOPMENT' "$HDR"
grep -q 'NOVA_APP_METADATA_FLAG_HAS_DESCRIPTION' "$HDR"
grep -q 'NOVA_APP_METADATA_FLAG_HAS_ICON' "$HDR"
grep -q 'struct nova_app_metadata_reply' "$HDR"
grep -q 'sizeof(struct nova_app_catalog_request) == 32' "$HDR"
grep -q 'sizeof(struct nova_app_catalog_reply) == 152' "$HDR"
grep -q 'sizeof(struct nova_app_metadata_reply) == 264' "$HDR"

# Rich metadata must remain presentation-only and pathless.
if grep -Eqi 'char[[:space:]]+(exec|path|argv|environment|command|uri)\[' "$HDR"; then
  echo 'FAIL: forbidden executable/path/command field in metadata ABI' >&2
  exit 1
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/check.c" <<'C'
#include "nova_app_metadata_protocol.h"
int main(void) {
    struct nova_app_catalog_request req = {0};
    struct nova_app_catalog_reply base = {0};
    struct nova_app_metadata_reply meta = {0};
    return (int)(sizeof(req) + sizeof(base) + sizeof(meta) == 448U ? 0 : 1);
}
C
clang -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/runtime" -fsyntax-only "$tmp/check.c"

echo 'Stage-5A Phase-15 additive rich application-metadata UAPI/Phase-14 compatibility contract: PASS'
