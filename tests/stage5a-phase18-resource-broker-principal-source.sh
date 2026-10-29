#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
N="$ROOT/src/init/main.c"
B="$ROOT/src/services/resource_broker.c"
H="$ROOT/src/runtime/nova_resource_broker_protocol.h"
M="$ROOT/configs/services/resource-broker.service"
L="$ROOT/configs/services/services.list"
BU="$ROOT/scripts/build-userland.sh"
BI="$ROOT/scripts/build-initramfs.sh"

for f in "$N" "$B" "$H" "$M" "$L" "$BU" "$BI"; do [[ -f "$f" ]] || { echo "missing Phase-18 file: $f" >&2; exit 1; }; done

grep -qx 'name=resource-broker' "$M"
grep -qx 'exec=/System/Core/nova-resource-broker' "$M"
! grep -qE '^(kind=application|app_id=|client_id=|capabilities=|sandbox=)' "$M"
grep -qx '/System/Services/resource-broker.service' "$L"
grep -Fq 'src/services/resource_broker.c" "$ROOTFS/System/Core/nova-resource-broker' "$BU"
grep -Fq 'resource-broker.service" "$ROOTFS/System/Services/resource-broker.service' "$BI"

for marker in \
  '#include "nova_resource_broker_protocol.h"' \
  'nova_getsockopt(fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED' \
  'lookup_live_principal(peer_pid,&principal)' \
  'NOVA_RESOURCE_BROKER_HOST_SOCKET_PATH' \
  'nova_fchmodat(NOVA_RESOURCE_BROKER_HOST_SOCKET_PATH,0666)' \
  'NOVA_RESOURCE_BROKER_OP_HELLO' \
  'NOVA_RESOURCE_BROKER_OP_SESSION' \
  'NOVA_RESOURCE_BROKER_FLAG_NO_GRANT_SURFACE' \
  'hello_grants=none' \
  'grant_surface=fixed-demo-readonly-fd'; do
  grep -Fq "$marker" "$B" || { echo "missing resource broker marker: $marker" >&2; exit 1; }
done

for marker in \
  'static int g_app_security_listener=-1;' \
  'find_service("compositor")' \
  'application security client is not the supervised compositor' \
  'static int g_resource_security_listener=-1;' \
  'setup_resource_security_listener' \
  'NOVA_RESOURCE_BROKER_AUTHORITY_SOCKET_PATH' \
  'find_service("resource-broker")' \
  'resource principal client is not the supervised resource-broker' \
  'process_app_security_packet(resource_security_client)' \
  'accept_resource_security_client(g_resource_security_listener,&resource_security_client)'; do
  grep -Fq "$marker" "$N" || { echo "missing novad broker-authority marker: $marker" >&2; exit 1; }
done

python3 - "$N" "$B" <<'PY'
import re,sys
novad=open(sys.argv[1],encoding='utf-8').read()
broker=open(sys.argv[2],encoding='utf-8').read()

def body(text,name):
    m=re.search(r'\b'+re.escape(name)+r'\s*\([^;]*?\)\s*\{',text,re.S)
    assert m,name
    i=m.end(); depth=1
    while i<len(text) and depth:
        if text[i]=='{': depth+=1
        elif text[i]=='}': depth-=1
        i+=1
    assert depth==0,name
    return text[m.start():i]

# Frozen Phase-16 authority remains compositor-only.
phase16=body(novad,'accept_app_security_client')
assert 'find_service("compositor")' in phase16
assert 'resource-broker' not in phase16

resource=body(novad,'accept_resource_security_client')
assert 'find_service("resource-broker")' in resource
assert 'peer_pid_for_socket' in resource
assert 'broker->pid' in resource

accept=body(broker,'accept_sessions')
assert 'peer_pid_for_fd' in accept
assert 'lookup_live_principal(peer_pid,&principal)' in accept
assert 'principal.client_id' in accept and 'principal.app_generation' in accept

lookup=body(broker,'lookup_live_principal')
assert 'NOVA_APP_SECURITY_OP_LOOKUP_PID' in lookup
assert 'reply->process_id!=peer_pid' in lookup
assert 'effective_capabilities&~reply->declared_capabilities' in lookup

packet=body(broker,'process_session_packet')
assert 'request.opcode!=NOVA_RESOURCE_BROKER_OP_HELLO' in packet
assert 'NOVA_RESOURCE_BROKER_RESULT_UNSUPPORTED' in packet
assert 'NOVA_RESOURCE_BROKER_OP_SESSION' in packet
PY

CC="${CC:-clang}"
for ARCH in x86_64 aarch64; do
  TARGET=x86_64-linux-gnu; MACRO=x86_64
  [[ "$ARCH" == aarch64 ]] && { TARGET=aarch64-linux-gnu; MACRO=aarch64; }
  "$CC" --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only \
    -I"$ROOT/src/runtime" \
    -DNOVA_VERSION=\"0.0.12-dev\" -DNOVA_BUILD_ARCH=\"$MACRO\" \
    -DNOVA_ROOT_CERT_SHA256=\"UNPINNED\" -DNOVA_TRUST_PROFILE=\"development\" \
    -DNOVA_TRUST_KEY_ID=\"nova-development-root-2026\" -DNOVA_UEFI_CERT_SHA256=\"UNPINNED\" \
    -DNOVA_RELEASE_KEY_ID=\"nova-development-release-2026-a\" -DNOVA_TRUST_EPOCH=1 \
    "$N" "$B"
done

echo 'Stage-5A Phase-18 SO_PEERCRED -> broker-only novad live-principal binding source contract: PASS'
