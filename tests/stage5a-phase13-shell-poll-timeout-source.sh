#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
S="$ROOT/src/services/shell.c"

python3 - "$S" <<'PY'
import sys
s=open(sys.argv[1],encoding='utf-8').read()
start=s.index('static int connect_once')
end=s.index('int nova_main', start)
block=s[start:end]
loop=block.index('for (;;)')
poll=block.index('nova_poll(', loop)
quarter=block.rfind('struct nova_timespec quarter = {0, 250000000L};', loop, poll)
assert quarter >= loop, 'poll timeout must be recreated inside the connection loop'
assert "Linux's raw ppoll syscall may modify the timeout argument" in block
prefix=block[:loop]
assert 'struct nova_timespec quarter = {0, 250000000L};' not in prefix
PY

CC="${CC:-clang}"
"$CC" -std=c11 -Wall -Wextra -Werror -fsyntax-only -I"$ROOT/src/runtime" "$S"

echo 'Stage-5A Phase-13 nova-shell per-ppoll timeout reset / heartbeat-backlog source contract: PASS'
