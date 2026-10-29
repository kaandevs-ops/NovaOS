#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HDR="$ROOT/src/runtime/nova_resource_broker_protocol.h"
[[ -f "$HDR" ]] || { echo 'FAIL: resource broker protocol header missing' >&2; exit 1; }

for marker in \
  'NOVA_RESOURCE_BROKER_APP_SOCKET_PATH "/run/nova/resource.sock"' \
  'NOVA_RESOURCE_BROKER_HOST_SOCKET_PATH "/run/nova/app/resource.sock"' \
  'NOVA_RESOURCE_BROKER_AUTHORITY_SOCKET_PATH "/run/nova/resource-authority.sock"' \
  'NOVA_RESOURCE_BROKER_PROTOCOL_MAGIC 0x4e524252U' \
  'NOVA_RESOURCE_BROKER_PROTOCOL_VERSION 1U' \
  'NOVA_RESOURCE_BROKER_FLAG_NO_GRANT_SURFACE' \
  'sizeof(struct nova_resource_broker_request) == 32' \
  'sizeof(struct nova_resource_broker_reply) == 96'; do
  grep -Fq "$marker" "$HDR" || { echo "missing Phase-18 resource broker UAPI marker: $marker" >&2; exit 1; }
done

python3 - "$HDR" <<'PY'
import re,sys
text=open(sys.argv[1],encoding='utf-8').read()
req=re.search(r'struct nova_resource_broker_request\s*\{(.*?)\};',text,re.S)
assert req
body=req.group(1)
for forbidden in ('process_id','client_id','app_id','path','uri','command','resource_id','fd'):
    assert forbidden not in body, f'caller authority leaked into request: {forbidden}'
assert body.count('reserved') == 3
assert 'NOVA_RESOURCE_BROKER_OP_HELLO = 1U' in text
assert 'filesystem.open' not in text
assert 'network.client' not in text
assert 'device.' not in text
PY

CC="${CC:-clang}"
for ARCH in x86_64 aarch64; do
  TARGET=x86_64-linux-gnu; MACRO=x86_64
  [[ "$ARCH" == aarch64 ]] && { TARGET=aarch64-linux-gnu; MACRO=aarch64; }
  cat <<'C' | "$CC" --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only -I"$ROOT/src/runtime" -x c -
#include "nova_resource_broker_protocol.h"
int probe(void) {
    struct nova_resource_broker_request r = {0};
    struct nova_resource_broker_reply p = {0};
    return (int)(sizeof(r) + sizeof(p));
}
C
done

echo 'Stage-5A Phase-18 bounded pathless resource-broker session UAPI contract: PASS'
