#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
H="$ROOT/src/runtime/nova_app_catalog_protocol.h"
L="$ROOT/src/runtime/nova_app_protocol.h"
D="$ROOT/docs/STAGE5A_PHASE14_APPLICATION_CATALOG.md"

for marker in \
  '#define NOVA_APP_CATALOG_SOCKET_PATH "/run/nova/app-catalog.sock"' \
  '#define NOVA_APP_CATALOG_PROTOCOL_MAGIC 0x4e434154U' \
  '#define NOVA_APP_CATALOG_PROTOCOL_VERSION 1U' \
  '#define NOVA_APP_DISPLAY_NAME_BYTES 64U' \
  '#define NOVA_APP_CATALOG_MAX_ENTRIES 64U' \
  'NOVA_APP_CATALOG_OP_HELLO = 1U' \
  'NOVA_APP_CATALOG_OP_GET_ENTRY = 2U' \
  'NOVA_APP_CATALOG_OP_ENTRY = 0x101U' \
  'NOVA_APP_CATALOG_OP_END = 0x102U' \
  'NOVA_APP_CATALOG_FLAG_VISIBLE = 1U << 0' \
  'NOVA_APP_CATALOG_FLAG_LAUNCHABLE = 1U << 1' \
  'struct nova_app_catalog_request {' \
  'struct nova_app_catalog_reply {' \
  'sizeof(struct nova_app_catalog_request) == 32' \
  'sizeof(struct nova_app_catalog_reply) == 152'; do
  grep -Fq "$marker" "$H" || { echo "missing catalog UAPI marker: $marker" >&2; exit 1; }
done

# Phase-13 lifecycle ABI is frozen and must not be repurposed for catalog payloads.
grep -Fq '#define NOVA_APP_PROTOCOL_VERSION 1U' "$L"
grep -Fq 'sizeof(struct nova_app_message) == 80' "$L"

python3 - "$H" "$D" <<'PY'
import re,sys
h=open(sys.argv[1],encoding='utf-8').read()
d=open(sys.argv[2],encoding='utf-8').read()
dl=d.lower()

req=re.search(r'struct nova_app_catalog_request\s*\{(.*?)\};',h,re.S)
rep=re.search(r'struct nova_app_catalog_reply\s*\{(.*?)\};',h,re.S)
assert req and rep

# Discovery must not become an executable-command transport.
for body in (req.group(1),rep.group(1)):
    low=body.lower()
    for forbidden in ('exec','path','argv','environment','command'):
        assert forbidden not in low, (forbidden,body)

# Exactly one fixed identity and one bounded presentation label per reply.
assert rep.group(1).count('char app_id[')==1
assert rep.group(1).count('char display_name[')==1
assert 'unsigned int client_id;' in rep.group(1)
assert 'unsigned int state;' in rep.group(1)
assert 'unsigned long long catalog_generation;' in rep.group(1)

# Scope document must explicitly preserve the core trust boundaries.
for marker in (
    'remains the authority for application registry parsing',
    'shell never chooses or sends an executable path',
    'compositor never becomes a process manager',
    'No runtime endpoint is added in Step-1.',
):
    assert marker.lower() in dl, marker
PY

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/probe.c" <<'C'
#include "nova_app_catalog_protocol.h"
int main(void) {
    struct nova_app_catalog_request q = {0};
    struct nova_app_catalog_reply r = {0};
    return (int)(sizeof(q) + sizeof(r) == 184U ? 0 : 1);
}
C
clang -std=c11 -Wall -Wextra -Werror -fsyntax-only -I"$ROOT/src/runtime" "$TMP/probe.c"

echo 'Stage-5A Phase-14 bounded/versioned read-only application catalog UAPI contract: PASS'
