#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/src/services/compositor.c"
python3 - "$P" <<'PY'
import sys
s=open(sys.argv[1],encoding='utf-8').read()
for cid in (1,2,3):
    assert f'client_id_is_connected({cid}U)' in s
assert 'g_connections[i].used&&g_connections[i].authenticated&&g_connections[i].client_id==id' in s
old='(g_connections[0].used || g_connections[1].used) && (g_connections[0].client_id=='
assert old not in s, 'cross-slot used/client_id connected-state bug still present'
PY
echo 'Stage-5A Phase-13 per-client connected-state slot binding source contract: PASS'
