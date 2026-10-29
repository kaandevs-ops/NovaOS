#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
L="$ROOT/src/runtime/nova_linux.h"
S="$ROOT/src/runtime/nova_syscall.h"
N="$ROOT/src/init/main.c"
C="$ROOT/src/services/compositor.c"

for marker in \
  'NOVA_SO_PEERCRED = 17' \
  'struct nova_ucred {' \
  '_Static_assert(sizeof(struct nova_ucred) == 12' \
  '#define NOVA_NR_getsockopt    55' \
  '#define NOVA_NR_getsockopt    209'; do
  grep -Fq "$marker" "$L" || { echo "missing Phase-16 peer-credential UAPI marker: $marker" >&2; exit 1; }
done
grep -Fq 'static inline long nova_getsockopt(' "$S"

for marker in \
  'static struct nova_service *find_application_by_pid(' \
  'static int peer_pid_for_socket(' \
  'nova_getsockopt(fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED' \
  'static int setup_app_security_listener(' \
  'NOVA_APP_SECURITY_SOCKET_PATH' \
  'static int app_security_request_valid(' \
  'static int process_app_security_packet(' \
  'NOVA_APP_SECURITY_OP_LOOKUP_PID' \
  'find_application_by_pid(request.process_id)' \
  'NOVA_APP_SECURITY_FLAG_AUTHENTICATED|NOVA_APP_SECURITY_FLAG_POLICY_BOUND' \
  'static void accept_app_security_client(' \
  'find_service("compositor")' \
  'application security client is not the supervised compositor' \
  'static int g_app_security_listener=-1;' \
  'struct nova_pollfd security_fds[2];' \
  'accept_app_security_client(g_app_security_listener,&security_client)' \
  'run_novad_event_loop(app_listener,catalog_listener)' \
  'nova_unlink(NOVA_APP_SECURITY_SOCKET_PATH)'; do
  grep -Fq "$marker" "$N" || { echo "missing Phase-16 novad principal-authority marker: $marker" >&2; exit 1; }
done

for marker in \
  '#include "nova_app_security_protocol.h"' \
  'unsigned int peer_pid;' \
  'unsigned int authenticated;' \
  'unsigned long long app_generation;' \
  'unsigned long long effective_capabilities;' \
  'static int peer_pid_for_fd(' \
  'nova_getsockopt(fd,NOVA_SOL_SOCKET,NOVA_SO_PEERCRED' \
  'static int connect_security_authority(' \
  'NOVA_APP_SECURITY_OP_HELLO' \
  'static int lookup_security_principal(' \
  'NOVA_APP_SECURITY_OP_LOOKUP_PID' \
  'NOVA_APP_CAP_UI_SURFACE' \
  'static int authenticate_client_connection(' \
  'principal.client_id!=claimed_client_id' \
  'm.opcode!=NOVA_SURFACE_OP_HELLO' \
  'reason=hello_required_before_authentication' \
  'reason=principal_authentication_failed' \
  'reason=client_id_principal_mismatch' \
  'authenticated_clients=' \
  '_peer_pid=' \
  '_authenticated=' \
  '_generation=' \
  '_capabilities='; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-16 compositor principal-binding marker: $marker" >&2; exit 1; }
done

python3 - "$N" "$C" <<'PY'
import re,sys
novad=open(sys.argv[1],encoding='utf-8').read()
comp=open(sys.argv[2],encoding='utf-8').read()

def body(text,name):
    m=re.search(r'static\s+[^\n]+\s+'+re.escape(name)+r'\s*\([^)]*\)\s*\{',text)
    assert m,name
    i=m.end(); depth=1
    while i<len(text) and depth:
        if text[i]=='{': depth+=1
        elif text[i]=='}': depth-=1
        i+=1
    assert depth==0,name
    return text[m.start():i]

accept=body(novad,'accept_app_security_client')
assert 'find_service("compositor")' in accept
assert 'peer_pid_for_socket' in accept
assert 'compositor->pid' in accept

lookup=body(novad,'process_app_security_packet')
assert 'normal_boot_policy_satisfied()' in lookup
assert 'find_application_by_pid(request.process_id)' in lookup
assert 'application_capability_policy_valid(svc)' in lookup

consume=body(comp,'consume_client_message')
assert 'm.opcode!=NOVA_SURFACE_OP_HELLO' in consume
assert 'authenticate_client_connection(conn,m.client_id)' in consume
# Client IDs must not be assigned directly from an unauthenticated message.
assert 'conn->client_id=m.client_id' not in consume

auth=body(comp,'authenticate_client_connection')
assert 'lookup_security_principal(conn->peer_pid' in auth
assert 'principal.client_id!=claimed_client_id' in auth
assert 'conn->client_id=principal.client_id' in auth
assert 'conn->app_generation=principal.app_generation' in auth
assert 'conn->effective_capabilities=principal.effective_capabilities' in auth

principal=body(comp,'lookup_security_principal')
assert 'effective_capabilities&~reply->declared_capabilities' in principal
assert 'security_flags!=(NOVA_APP_SECURITY_FLAG_AUTHENTICATED|NOVA_APP_SECURITY_FLAG_POLICY_BOUND)' in principal

auth=body(comp,'authenticate_client_connection')
assert 'NOVA_APP_CAP_UI_SURFACE' in auth
assert 'principal.effective_capabilities' in auth
PY

CC="${CC:-clang}"
for ARCH in x86_64 aarch64; do
  TARGET="x86_64-linux-gnu"; MACRO=x86_64
  [[ "$ARCH" == aarch64 ]] && { TARGET="aarch64-linux-gnu"; MACRO=aarch64; }
  "$CC" --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only \
    -I"$ROOT/src/runtime" \
    -DNOVA_VERSION=\"0.0.12-dev\" -DNOVA_BUILD_ARCH=\"$MACRO\" \
    -DNOVA_ROOT_CERT_SHA256=\"UNPINNED\" -DNOVA_TRUST_PROFILE=\"development\" \
    -DNOVA_TRUST_KEY_ID=\"nova-development-root-2026\" -DNOVA_UEFI_CERT_SHA256=\"UNPINNED\" \
    -DNOVA_RELEASE_KEY_ID=\"nova-development-release-2026-a\" -DNOVA_TRUST_EPOCH=1 \
    "$N" "$C"
done

echo 'Stage-5A Phase-16 kernel SO_PEERCRED -> novad live principal -> compositor client binding source contract: PASS'
