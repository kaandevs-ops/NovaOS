#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; A="$ROOT/src/services/demo_app.c"
for marker in \
  'NOVA_SURFACE_OP_SET_ROLE' 'NOVA_SURFACE_ROLE_TOPLEVEL' 'NOVA_SURFACE_OP_SET_TITLE' 'NOVA_SURFACE_OP_SET_SIZE_LIMITS' \
  'NOVA_SURFACE_EVENT_CONFIGURE' 'NOVA_SURFACE_OP_ACK_CONFIGURE' 'replace_buffer_for_configure' \
  'register_buffer(fd,&fresh' 'NOVA_SURFACE_OP_ATTACH_BUFFER' 'NOVA_SURFACE_OP_DAMAGE' 'NOVA_SURFACE_OP_COMMIT' \
  'send_destroy_buffer(fd,old_id' 'NOVA_SURFACE_EVENT_CLOSE_REQUEST' 'NOVA_SURFACE_OP_DESTROY_SURFACE' \
  'action=destroy-surface-buffer' 'stage=CONNECTED protocol=6 surfaces=2 buffers=2 roles=toplevel widgets=3 textedit=selection-clipboard transport=memfd-scm-rights'; do
  grep -Fq "$marker" "$A" || { echo "missing Phase-7 demo-app marker: $marker" >&2; exit 1; }
done
echo 'Stage-5A Phase-7 demo-app configure/ACK/new-buffer/close lifecycle contract: PASS'
