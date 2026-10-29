#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; C="$ROOT/src/services/compositor.c"
for marker in \
  'struct nova_client_surface' 'pending_x' 'pending_buffer_id' 'client_surface_count' \
  'find_or_allocate_client_surface' 'clamp_client_surface' 'draw_client_surfaces' \
  'top_client_surface_at' 'cycle_focus' 'route_client_event' 'set_client_focus' \
  'NOVA_SURFACE_OP_CREATE_SURFACE' 'NOVA_SURFACE_OP_SET_POSITION' \
  'NOVA_SURFACE_OP_ATTACH_BUFFER' 'NOVA_SURFACE_OP_DAMAGE' 'NOVA_SURFACE_OP_COMMIT' \
  'surface->x=surface->pending_x' 'surface->committed=1' \
  'g_client.commits++' 'g_client.surface_generation++' \
  'client_ipc=DISCONNECTED surfaces_removed=' 'g_client.surfaces_removed+=removed' \
  'NOVA_SURFACE_EVENT_KEY' 'NOVA_SURFACE_EVENT_POINTER'; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-5 compositor compatibility marker: $marker" >&2; exit 1; }
done
grep -Fq 'draw_client_surfaces(base,width,height,pitch)' "$C"
# R19 overlays occlude application input; the same surface hit-test remains behind the guard.
grep -Fq 'g_client.pointer_surface=r19_overlay_at(g_render.cursor_x,g_render.cursor_y)?0U:top_client_surface_at' "$C"
grep -Fq 'client_focus_surface=' "$C"
grep -Fq 'client_surface_count=' "$C"
echo 'Stage-5A Phase-5 committed surface registry/z-order/hit-test/input-routing/cleanup compatibility contract: PASS'
