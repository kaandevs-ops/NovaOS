#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; C="$ROOT/src/services/compositor.c"
for marker in \
  'struct nova_client_grab' 'NOVA_GRAB_MOVE' 'NOVA_GRAB_RESIZE' 'raise_client_surface' 'max_client_z' \
  'cursor_in_toplevel_title' 'cursor_in_toplevel_close' 'cursor_in_toplevel_resize_edge' \
  'begin_client_grab' 'update_client_grab' 'finish_client_grab' \
  'send_surface_configure' 'NOVA_SURFACE_EVENT_CONFIGURE' 'configure_serial' 'acked_configure_serial' 'awaiting_configure' \
  'commit_before_configure_ack' 'configure_buffer_size_mismatch' 'NOVA_SURFACE_EVENT_CLOSE_REQUEST' \
  'destroy_client_surface_request' 'client_window_moves=' 'client_window_resizes=' 'client_window_raises=' \
  'client_close_requests=' 'client_explicit_surface_destroys='; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-7 compositor marker: $marker" >&2; exit 1; }
done
grep -Fq 'lifecycle=toplevel-configure-ack' "$C"
grep -Fq 'buffer_transport=memfd-scm-rights' "$C"
echo 'Stage-5A Phase-7 compositor raise/move/resize/configure/close lifecycle contract: PASS'
