#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
S="$ROOT/src/services/shell.c"

for marker in \
  '#include "nova_app_protocol.h"' \
  'NOVA_APP_SOCKET_PATH' \
  'lifecycle_request' \
  'NOVA_APP_OP_HELLO' \
  'NOVA_APP_OP_LAUNCH' \
  'NOVA_APP_OP_STOP' \
  'lifecycle_stop' \
  'NOVA_APP_RESULT_ALREADY_RUNNING' \
  'launcher=NOVAD_PROCESS_SPAWN' \
  'launcher=RESIDENT_WINDOW_FALLBACK' \
  'app_lifecycle_connected=' \
  'app_lifecycle=CONNECTED protocol=1 launcher=novad-process-v1 close=graceful-stop-v1 compatibility=resident-window-v1'; do
  grep -Fq "$marker" "$S" || { echo "missing Phase-13 shell lifecycle marker: $marker" >&2; exit 1; }
done

python3 - "$S" <<'PY'
import sys
s=open(sys.argv[1],encoding='utf-8').read()
start=s.index('static int respond_to_task_event')
end=s.index('static int connect_once',start)
block=s[start:end]
assert 'NOVA_DESKTOP_ACTION_LAUNCH' in block
assert 'lifecycle_launch(' in block
assert 'status.result == NOVA_APP_RESULT_OK' in block
assert 'status.result == NOVA_APP_RESULT_ALREADY_RUNNING' in block
assert 'NOVA_DESKTOP_OP_LAUNCH' in block
assert 'NOVA_DESKTOP_ACTION_CLOSE' in block
assert 'lifecycle_stop(' in block
assert 'NOVA_APP_STATE_STOPPING' in block
assert block.index('lifecycle_stop(') < block.index('NOVA_DESKTOP_OP_CLOSE')
# Phase-14 may replace the original fixed app-id mapping, but Phase-13's
# invariant remains: lifecycle control transports registry app IDs, never paths.
assert '/System/Apps/' not in s
launch=s[s.index('static int lifecycle_launch'):s.index('static int lifecycle_stop')]
stop=s[s.index('static int lifecycle_stop'):s.index('static int respond_to_task_event')]
assert 'NOVA_APP_OP_LAUNCH' in launch and 'app_id' in launch
assert 'NOVA_APP_OP_STOP' in stop and 'app_id' in stop
PY

CC="${CC:-clang}"
"$CC" -std=c11 -Wall -Wextra -Werror -fsyntax-only \
  -I"$ROOT/src/runtime" "$S"

echo 'Stage-5A Phase-13 nova-shell lifecycle launch/stop + graceful-close policy source contract: PASS'
