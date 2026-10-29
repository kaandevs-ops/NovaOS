#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; C="$ROOT/src/services/compositor.c"
for marker in \
  'struct nova_client_buffer' 'g_client_buffers[NOVA_SURFACE_MAX_CLIENT_BUFFERS]' 'client_buffer_count' 'find_client_buffer' \
  'recv_surface_packet' 'nova_recvmsg' 'NOVA_MSG_CMSG_CLOEXEC' 'NOVA_MSG_CTRUNC|NOVA_MSG_TRUNC' 'NOVA_SCM_RIGHTS' \
  'validate_client_buffer_descriptor' 'NOVA_PIXEL_FORMAT_XRGB8888' 'NOVA_SURFACE_MAX_BUFFER_BYTES' \
  'nova_lseek(received_fd,0,NOVA_SEEK_END)' 'buffer_backing_size_mismatch' \
  'r17_refresh_client_buffer' 'NOVA_MAP_PRIVATE|NOVA_MAP_ANONYMOUS' 'buffer_snapshot_read_failed' 'buffer_not_regular' 'buffer_not_owned' 'damage_rect_invalid' 'commit_without_owned_buffer' \
  'blit_client_buffer' 'src_row[x]&0x00ffffffU' 'destroy_client_buffer_entry' \
  'buffers_removed=' 'client_buffer_count=' 'client_buffers_created=' 'client_buffers_removed='; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-6 compositor buffer marker: $marker" >&2; exit 1; }
done
grep -Fq 'buffer_transport=memfd-scm-rights' "$C"
grep -Fq 'client_ipc=LISTENING protocol=' "$C"
grep -Fq 'client_ipc=CONNECTED protocol=' "$C"
echo 'Stage-5A Phase-6 compositor SCM_RIGHTS buffer registry/ownership/validation/blit/cleanup contract: PASS'
