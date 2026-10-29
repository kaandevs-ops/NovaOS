#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/src/services/compositor.c"
python3 - "$P" <<'PY'
import re, sys
s=open(sys.argv[1], encoding='utf-8').read()
loop=s[s.index('if (shell_client>=0) {'):s.index('/* Process old-client HUP before accepting replacements')]
checks=[
    r'disconnect_shell_client\(&shell_client\);\s*write_state\(',
    r'disconnect_desktop_client\(&desktop_client\);\s*write_state\(',
    r'if\(consume_app_pollfds\(pollfds,app_client_base,app_clients,.*?\)\)\s*write_state\(',
]
for pat in checks:
    if not re.search(pat, loop, re.S):
        raise SystemExit(f'missing disconnect state publication contract: {pat}')
helper=s[s.index('static int consume_app_pollfds('):s.index('static unsigned int open_input_set(')]
assert 'changed=1;disconnect_client_connection(&clients[ci]);' in helper
assert 'return changed;' in helper
if 'client2_connected=' not in s or 'client2_surface_count=' not in s or 'client2_buffer_count=' not in s:
    raise SystemExit('per-client machine-readable compositor state markers missing')
PY

echo 'Stage-5A Phase-13 compositor disconnect -> machine-readable state publication source contract: PASS'
