#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
M="$ROOT/src/init/main.c"
H="$ROOT/src/runtime/nova_app_catalog_protocol.h"
A="$ROOT/configs/services/demo-app.service"
B="$ROOT/configs/services/demo-app-b.service"

for marker in \
  '#include "nova_app_catalog_protocol.h"' \
  '#define NOVA_APP_CATALOG_STATE_FILE "/run/nova/app-catalog.state"' \
  'char display_name[NOVA_APP_DISPLAY_NAME_BYTES];' \
  'static unsigned long long g_app_catalog_generation = 0ULL;' \
  'static int safe_display_name(' \
  'static unsigned int application_catalog_count(void)' \
  'static struct nova_service *application_catalog_entry(unsigned int index)' \
  'static void write_app_catalog_state(void)' \
  'static int setup_app_catalog_listener(void)' \
  'static int app_catalog_request_valid(' \
  'static int process_app_catalog_packet(int fd)' \
  'static void accept_app_catalog_client(' \
  'static void disconnect_app_catalog_client(' \
  'static void run_novad_event_loop(int app_listener,int catalog_listener)' \
  'struct nova_pollfd fds[5];' \
  'long rc=nova_poll(fds,5,&timeout);' \
  'int catalog_listener=setup_app_catalog_listener();' \
  'run_novad_event_loop(app_listener,catalog_listener);' \
  '(void)nova_unlink(NOVA_APP_CATALOG_SOCKET_PATH);'; do
  grep -Fq "$marker" "$M" || { echo "missing novad catalog marker: $marker" >&2; exit 1; }
done

for manifest in "$A" "$B"; do
  grep -qx 'kind=application' "$manifest"
  grep -Eq '^display_name=[[:print:]]+$' "$manifest"
  grep -Eq '^app_id=[A-Za-z0-9._-]+$' "$manifest"
  grep -Eq '^client_id=[1-9][0-9]*$' "$manifest"
  grep -qx 'autostart=no' "$manifest"
done
grep -qx 'display_name=Nova Demo A' "$A"
grep -qx 'display_name=Nova Demo B' "$B"

python3 - "$M" "$H" <<'PY'
import re,sys
m=open(sys.argv[1],encoding='utf-8').read()
h=open(sys.argv[2],encoding='utf-8').read()

def body(name):
    mm=re.search(r'static\s+[^\n]+\s+'+re.escape(name)+r'\s*\([^)]*\)\s*\{',m)
    assert mm,name
    i=mm.end(); depth=1
    while i<len(m) and depth:
        if m[i]=='{': depth+=1
        elif m[i]=='}': depth-=1
        i+=1
    assert depth==0,name
    return m[mm.start():i]

# Producer must be a projection of novad's registry, not a path/command transport.
reply=body('fill_app_catalog_reply').lower()
state=body('write_app_catalog_state').lower()
for forbidden in ('svc->exec','argv','environment','shell command'):
    assert forbidden not in reply, forbidden
    assert forbidden not in state, forbidden
for required in ('svc->app_id','svc->display_name','svc->client_id','svc->app_state'):
    assert required in reply, required

# Strict fixed-frame validation and fail-closed authorization.
valid=body('app_catalog_request_valid')
for marker in ('NOVA_APP_CATALOG_PROTOCOL_MAGIC','NOVA_APP_CATALOG_PROTOCOL_VERSION',
               'sizeof(*request)','request->flags!=0U','request->reserved!=0U'):
    assert marker in valid, marker
proc=body('process_app_catalog_packet')
for marker in ('NOVA_MSG_TRUNC','NOVA_MSG_CTRUNC','NOVA_APP_CATALOG_RESULT_BAD_REQUEST',
               'normal_boot_policy_satisfied()','NOVA_APP_CATALOG_RESULT_DENIED',
               'NOVA_APP_CATALOG_OP_HELLO_REPLY','NOVA_APP_CATALOG_OP_ENTRY','NOVA_APP_CATALOG_OP_END'):
    assert marker in proc, marker

# Catalog is bounded by the ABI limit even if MAX_SERVICES grows later.
consistent=body('application_registry_consistent')
assert 'application_catalog_count()>NOVA_APP_CATALOG_MAX_ENTRIES' in consistent

# Phase-13 lifecycle socket remains independently wired.
loop=body('run_novad_event_loop')
for marker in ('process_app_lifecycle_packet(app_client)','process_app_catalog_packet(catalog_client)',
               'accept_app_lifecycle_client(app_listener','accept_app_catalog_client(catalog_listener'):
    assert marker in loop,marker

# Lifecycle ABI must still be a separate include/type and is not replaced by catalog frames.
assert '#include "nova_app_protocol.h"' in m
assert '#include "nova_app_catalog_protocol.h"' in m
assert 'struct nova_app_message' in m
assert 'struct nova_app_catalog_request' in m
assert 'NOVA_APP_CATALOG_MAX_ENTRIES 64U' in h
PY

echo 'Stage-5A Phase-14 novad trusted-manifest/read-only catalog producer source contract: PASS'
