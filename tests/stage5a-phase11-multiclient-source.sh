#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"; A="$ROOT/src/services/demo_app.c"; B="$ROOT/scripts/build-userland.sh"; I="$ROOT/scripts/build-initramfs.sh"
for marker in \
  'NOVA_MAX_CLIENT_CONNECTIONS 3U' 'struct nova_client_connection' 'g_connections[NOVA_MAX_CLIENT_CONNECTIONS]' \
  'connection_for_fd' 'client_connection_count' 'client_surface_count_for' 'client_buffer_count_for' \
  'surface->owner_fd' 'surface->client_id' 'buf->owner_fd!=client' 'g_client_buffers[i].client_id==client_id' \
  'ev.client_id=surface->client_id' 'nova_write(surface->owner_fd' \
  'client1_commits=' 'client2_commits=' 'client1_surface_count=' 'client2_surface_count=' \
  'client1_buffer_count=' 'client2_buffer_count=' 'client_id_in_use' 'connection_slots_full'; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-11 compositor multi-client marker: $marker" >&2; exit 1; }
done
for marker in \
  '#ifdef NOVA_DEMO_APP_B' 'APP_CLIENT_ID 2U' 'SURFACE_A 200U' 'SURFACE_B 201U' \
  'APP_STATE "/run/nova/demo-app-b.state"' 'APP_INITIAL_TEXT "BETA"' \
  'BUFFER_A 1U' 'BUFFER_B 2U'; do
  grep -Fq "$marker" "$A" || { echo "missing Phase-11 App-B marker: $marker" >&2; exit 1; }
done
grep -Fq 'nova-demo-app-b" -DNOVA_DEMO_APP_B=1' "$B"
grep -Fq 'demo-app-b.service' "$I"
grep -qx 'name=demo-app-b' "$ROOT/configs/services/demo-app-b.service"
grep -qx 'exec=/System/Apps/nova-demo-app-b' "$ROOT/configs/services/demo-app-b.service"
grep -qx '/System/Services/demo-app-b.service' "$ROOT/configs/services/services.list"
python3 - "$C" <<'PY_SLOTS'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
assert 'for(unsigned int ci=0;ci<NOVA_MAX_CLIENT_CONNECTIONS;ci++)app_clients[ci]=-1;' in s
assert 'g_client_commits_by_id[4]' in s
PY_SLOTS
echo 'Stage-5A Phase-11 two-client connection/surface/buffer ownership isolation source contract: PASS'
