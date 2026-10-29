#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/src/runtime/nova_desktop_protocol.h"
S="$ROOT/src/services/shell.c"
C="$ROOT/src/services/compositor.c"
for marker in \
  'NOVA_DESKTOP_OP_TASK_MODEL_ICON = 10U' \
  'NOVA_DESKTOP_TASK_FLAG_HAS_ICON' \
  '#define NOVA_DESKTOP_TASK_ICON_BYTES 576U' \
  'sizeof(struct nova_desktop_message) == 48' \
  'sizeof(struct nova_desktop_task_message) == 112' \
  'sizeof(struct nova_desktop_task_icon_message) == 712'; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-15 rich launcher ABI marker: $marker" >&2; exit 1; }
done
for marker in \
  '#include "nova_asset_protocol.h"' \
  'resolve_catalog_assets' \
  'NOVA_ASSET_OP_GET_INFO' \
  'NOVA_ASSET_OP_READ' \
  'nova_sha256_update' \
  'bytes_equal(digest, info.digest' \
  'NOVA_DESKTOP_TASK_FLAG_HAS_ICON' \
  'send_desktop_task_icon_message' \
  'connect_path(NOVA_ASSET_SOCKET_PATH)' \
  'app_assets=CONNECTED protocol=1' \
  'catalog_entry' \
  '_icon_ready='; do
  grep -Fq "$marker" "$S" || { echo "missing Phase-15 shell asset-resolution marker: $marker" >&2; exit 1; }
done
for forbidden in '/System/Icons' 'file:' 'http://' 'https://'; do
  ! grep -Fq "$forbidden" "$S" || { echo "forbidden path/URI in shell asset resolver: $forbidden" >&2; exit 1; }
done
for marker in \
  'consume_desktop_task_icon_message' \
  'icon-binding-invalid' \
  'reason=icon-missing' \
  'draw_task_icon' \
  'desktop_task_icon=STAGED' \
  '_icon_asset_id=' \
  'NOVA_DESKTOP_TASK_FLAG_HAS_ICON'; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-15 compositor rich-task marker: $marker" >&2; exit 1; }
done
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/check.c" <<'C'
#include "nova_desktop_protocol.h"
_Static_assert(sizeof(struct nova_desktop_message)==48,"control drift");
_Static_assert(sizeof(struct nova_desktop_task_message)==112,"task drift");
_Static_assert(sizeof(struct nova_desktop_task_icon_message)==712,"icon drift");
int main(void){return 0;}
C
clang -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/runtime" -fsyntax-only "$TMP/check.c"
echo 'Stage-5A Phase-15 verified asset-resolution + additive rich task-icon/render source contract: PASS'
