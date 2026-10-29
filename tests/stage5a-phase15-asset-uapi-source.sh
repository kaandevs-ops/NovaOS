#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
H="$ROOT/src/runtime/nova_asset_protocol.h"
[[ -f "$H" ]]
grep -q '#define NOVA_ASSET_SOCKET_PATH "/run/nova/assets.sock"' "$H"
grep -q '#define NOVA_ASSET_PROTOCOL_VERSION 1U' "$H"
grep -q '#define NOVA_ASSET_ID_BYTES 48U' "$H"
grep -q '#define NOVA_ASSET_CHUNK_BYTES 1024U' "$H"
grep -q 'NOVA_ASSET_OP_GET_INFO = 2U' "$H"
grep -q 'NOVA_ASSET_OP_READ = 3U' "$H"
grep -q 'NOVA_ASSET_FORMAT_A8_MASK = 1U' "$H"
grep -q 'sizeof(struct nova_asset_request) == 80' "$H"
grep -q 'sizeof(struct nova_asset_reply) == 1160' "$H"
! grep -Eq '(^|[^A-Za-z_])(path|uri|url|exec|argv|environment|command)[[:space:]]*\[' "$H"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/check.c" <<'C'
#include "nova_asset_protocol.h"
int main(void){ return sizeof(struct nova_asset_request)==80 && sizeof(struct nova_asset_reply)==1160 ? 0 : 1; }
C
clang -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/runtime" -fsyntax-only "$TMP/check.c"
echo 'Stage-5A Phase-15 bounded read-only trusted-asset UAPI/no-path contract: PASS'
