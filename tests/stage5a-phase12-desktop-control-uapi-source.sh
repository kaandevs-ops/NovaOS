#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/src/runtime/nova_desktop_protocol.h"; S="$ROOT/src/runtime/nova_surface_protocol.h"
grep -Fq '#define NOVA_DESKTOP_SOCKET_PATH "/run/nova/desktop.sock"' "$P"
grep -Fq '#define NOVA_DESKTOP_PROTOCOL_VERSION 1U' "$P"
for marker in \
  'NOVA_DESKTOP_OP_TASK_EVENT' 'NOVA_DESKTOP_OP_ACTIVATE' 'NOVA_DESKTOP_OP_CLOSE' 'NOVA_DESKTOP_OP_LAUNCH' \
  'NOVA_DESKTOP_ACTION_ACTIVATE' 'NOVA_DESKTOP_ACTION_CLOSE' 'NOVA_DESKTOP_ACTION_LAUNCH' \
  'sizeof(struct nova_desktop_message) == 48'; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-12 desktop UAPI marker: $marker" >&2; exit 1; }
done
grep -Fq 'NOVA_SURFACE_EVENT_LAUNCH = 0x106U' "$S"
echo 'Stage-5A Phase-12 desktop-control taskbar/launcher UAPI ABI contract: PASS'
