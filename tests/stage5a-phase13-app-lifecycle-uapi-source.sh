#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/src/runtime/nova_app_protocol.h"

for marker in \
  '#define NOVA_APP_SOCKET_PATH "/run/nova/app-lifecycle.sock"' \
  '#define NOVA_APP_PROTOCOL_VERSION 1U' \
  '#define NOVA_APP_ID_BYTES 32U' \
  'NOVA_APP_OP_LAUNCH' \
  'NOVA_APP_OP_STOP' \
  'NOVA_APP_OP_QUERY' \
  'NOVA_APP_OP_STATUS' \
  'NOVA_APP_STATE_STOPPED' \
  'NOVA_APP_STATE_STARTING' \
  'NOVA_APP_STATE_RUNNING' \
  'NOVA_APP_STATE_STOPPING' \
  'NOVA_APP_STATE_RESTARTING' \
  'NOVA_APP_STATE_FAILED' \
  'NOVA_APP_RESULT_UNKNOWN_APP' \
  'NOVA_APP_RESULT_ALREADY_RUNNING' \
  'NOVA_APP_RESULT_NOT_RUNNING' \
  'NOVA_APP_RESULT_SPAWN_FAILED' \
  'NOVA_APP_RESULT_STOP_FAILED' \
  'char app_id[NOVA_APP_ID_BYTES]' \
  'sizeof(struct nova_app_message) == 80'
do
    grep -Fq "$marker" "$P" || {
        echo "missing Phase-13 app lifecycle UAPI marker: $marker" >&2
        exit 1
    }
done

python3 - "$P" <<'PY_CHECK'
import re
import sys

source = open(sys.argv[1], encoding="utf-8").read()

match = re.search(
    r"struct nova_app_message\s*\{(.*?)\};",
    source,
    re.S,
)

assert match, "nova_app_message missing"

body = match.group(1)

for forbidden in (
    "exec",
    "argv",
    "environment",
    "command",
    "executable_path",
):
    assert forbidden not in body.lower(), (
        f"forbidden lifecycle transport field: {forbidden}"
    )

assert "app_id" in body
PY_CHECK

CC="${CC:-clang}"

printf '%s\n' \
  '#include "nova_app_protocol.h"' \
  'int main(void) { struct nova_app_message m = {0}; return (int)m.size; }' |
"$CC" \
  -std=c11 \
  -Wall \
  -Wextra \
  -Werror \
  -fsyntax-only \
  -I"$ROOT/src/runtime" \
  -x c -

echo 'Stage-5A Phase-13 bounded/versioned application-lifecycle UAPI ABI contract: PASS'
