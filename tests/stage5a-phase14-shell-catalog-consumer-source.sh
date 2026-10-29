#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
S="$ROOT/src/services/shell.c"

for marker in \
  '#include "nova_app_catalog_protocol.h"' \
  'struct shell_catalog_entry' \
  'struct shell_catalog_state' \
  'entries[NOVA_APP_CATALOG_MAX_ENTRIES]' \
  'catalog_entry_for_client' \
  'catalog_cache_conflicts' \
  'catalog_reply_header_valid' \
  'catalog_request' \
  'catalog_enumerate' \
  'NOVA_APP_CATALOG_SOCKET_PATH' \
  'NOVA_APP_CATALOG_OP_HELLO' \
  'NOVA_APP_CATALOG_OP_GET_ENTRY' \
  'NOVA_APP_CATALOG_OP_ENTRY' \
  'NOVA_APP_CATALOG_OP_END' \
  'app_catalog=ENUMERATED' \
  'app_catalog=CONNECTED protocol=1' \
  'app_catalog_connected=' \
  'app_catalog_count=' \
  'catalog_entry' \
  '_id='; do
  grep -Fq "$marker" "$S" || { echo "missing Phase-14 shell catalog marker: $marker" >&2; exit 1; }
done

python3 - "$S" <<'PY'
import re,sys
s=open(sys.argv[1],encoding='utf-8').read()
assert '#define SHELL_APP_A_ID' not in s
assert '#define SHELL_APP_B_ID' not in s
assert 'app_id_for_client' not in s
assert '"nova.demo.a"' not in s and '"nova.demo.b"' not in s
assert '/System/Apps/' not in s

catalog=s[s.index('static int catalog_enumerate'):s.index('static int lifecycle_launch')]
for token in ['NOVA_APP_CATALOG_OP_HELLO','NOVA_APP_CATALOG_OP_GET_ENTRY','NOVA_APP_CATALOG_OP_ENTRY','NOVA_APP_CATALOG_OP_END','NOVA_APP_CATALOG_MAX_ENTRIES']:
    assert token in s
assert 'expected_count = reply.total_count' in catalog
assert 'reply.total_count != expected_count' in catalog
assert 'reply.catalog_generation != expected_generation' in catalog
assert 'catalog_cache_conflicts' in catalog
assert 'catalog_app_id_valid' in catalog
assert 'catalog_display_name_valid' in catalog
assert 'g_catalog.count++' in catalog

respond=s[s.index('static int respond_to_task_event'):s.index('static int connect_once')]
assert 'catalog_entry_for_client(event->client_id)' in respond
assert respond.index('catalog_entry_for_client(event->client_id)') < respond.index('NOVA_DESKTOP_ACTION_ACTIVATE')
assert 'client-not-in-catalog' in respond
assert 'NOVA_APP_CATALOG_FLAG_LAUNCHABLE' in respond

launch=s[s.index('static int lifecycle_launch'):s.index('static int lifecycle_stop')]
stop=s[s.index('static int lifecycle_stop'):s.index('static int respond_to_task_event')]
assert 'entry->app_id' in launch and 'entry->display_name' not in launch
assert 'entry->app_id' in stop and 'entry->display_name' not in stop

connect=s[s.index('static int connect_once'):s.index('int nova_main')]
assert 'connect_path(NOVA_APP_CATALOG_SOCKET_PATH)' in connect
assert 'catalog_enumerate((int)catalog_fd' in connect
assert 'struct nova_pollfd pfds[' in connect
assert 'pfds[2].fd = (int)catalog_fd' in connect
assert 'NOVA_POLLERR | NOVA_POLLHUP' in connect
assert '(void)nova_close((int)catalog_fd)' in connect
assert 'g_catalog.reconnects++' in connect

state=s[s.index('static void write_state'):s.index('static int send_scene_message')]
assert 'schema=3' in state
for key in ['app_catalog_connected=','app_catalog_protocol=','app_catalog_enumerations=','app_catalog_count=','app_catalog_generation=','_display_name=']:
    assert key in state
PY

CC="${CC:-clang}"
"$CC" -std=c11 -Wall -Wextra -Werror -fsyntax-only \
  -I"$ROOT/src/runtime" "$S"

echo 'Stage-5A Phase-14 nova-shell bounded catalog discovery/cache/reconnect source contract: PASS'
