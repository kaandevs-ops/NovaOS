#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"
P16="$ROOT/tests/stage5a-phase16-peer-principal-source.sh"

python3 - "$C" <<'PY'
from pathlib import Path
import re, sys
s=Path(sys.argv[1]).read_text()
m=re.search(r'static int setup_sandbox_client_listener\(void\) \{(?P<body>.*?)\n\}', s, re.S)
assert m, 'sandbox listener function missing'
b=m.group('body')
bind=b.find('nova_bind((int)fd,&addr,sizeof(addr))')
mode=b.find('nova_fchmodat(NOVA_SURFACE_SANDBOX_SOCKET_PATH,0666)')
listen=b.find('nova_listen((int)fd,16)')
assert -1 not in (bind,mode,listen), 'bind/mode/listen markers missing'
assert bind < mode < listen, 'socket permission must be applied after bind and before listen'
assert 'reason=socket_mode_failed' in b, 'socket mode failure must be explicit'
assert 'write_sandbox_ipc_state("LISTENING","none",(int)fd)' in b, 'listener readiness state missing'
assert 'transport_auth=SO_PEERCRED_NOVAD_PRINCIPAL' in s, 'transport/auth boundary state missing'
assert 'nova_unlink(NOVA_SURFACE_SANDBOX_SOCKET_PATH)' in b[mode:], 'mode failure must unlink pathname'
assert 'SO_PEERCRED -> novad live-principal' in b, 'transport-vs-authentication invariant comment missing'
PY

# Transport mode must never replace authenticated principal binding.
"$P16" >/dev/null

echo 'Stage-5A Phase-17 unprivileged app-facing AF_UNIX transport permission + authenticated-principal boundary source contract: PASS'
