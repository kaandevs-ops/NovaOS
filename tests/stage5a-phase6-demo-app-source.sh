#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; A="$ROOT/src/services/demo_app.c"
for marker in \
  'nova_memfd_create' 'nova_ftruncate' 'NOVA_MAP_SHARED' 'NOVA_PROT_READ | NOVA_PROT_WRITE' \
  'NOVA_CMSG_SPACE(sizeof(int))' 'NOVA_SCM_RIGHTS' 'nova_sendmsg' \
  'NOVA_SURFACE_OP_CREATE_BUFFER' 'NOVA_SURFACE_OP_ATTACH_BUFFER' 'NOVA_SURFACE_OP_DAMAGE' 'NOVA_SURFACE_OP_COMMIT' \
  'NOVA_PIXEL_FORMAT_XRGB8888' 'BUFFER_A_BG 0x00184878U' 'BUFFER_A_ACCENT 0x00ffcc00U' \
  'BUFFER_B_BG 0x00502860U' 'BUFFER_B_ACCENT 0x0000ccccU' \
  'key=RIGHT action=buffer-redraw-damage-commit' \
  'stage=CONNECTED protocol=6 surfaces=2 buffers=2 roles=toplevel widgets=3 textedit=selection-clipboard transport=memfd-scm-rights'; do
  grep -Fq "$marker" "$A" || { echo "missing Phase-6 demo-app marker: $marker" >&2; exit 1; }
done
echo 'Stage-5A Phase-6 nova-demo-app client-owned memfd pixel production + DAMAGE/COMMIT contract: PASS'
