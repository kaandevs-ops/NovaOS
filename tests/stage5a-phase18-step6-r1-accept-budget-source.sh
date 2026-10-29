#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
B="$ROOT/src/services/resource_broker.c"
H="$ROOT/src/runtime/nova_resource_broker_protocol.h"
[[ -f "$B" && -f "$H" ]]

python3 - "$B" "$H" <<'PYTHON'
from pathlib import Path
import re,sys
s=Path(sys.argv[1]).read_text()
h=Path(sys.argv[2]).read_text()
assert re.search(r'#define\s+NOVA_RESOURCE_BROKER_MAX_CLIENTS\s+4U\b',h), 'client limit changed: reassess bound'

def func(name):
    m=re.search(r'\b'+re.escape(name)+r'\s*\([^;]*?\)\s*\{',s,re.S)
    assert m, name
    i=m.end(); depth=1
    while i<len(s) and depth:
        if s[i]=='{': depth+=1
        elif s[i]=='}': depth-=1
        i+=1
    assert depth==0,name
    return s[m.start():i]
accept=func('accept_sessions')
assert 'for(unsigned int accepted=0U;accepted<NOVA_RESOURCE_BROKER_MAX_CLIENTS;accepted++){' in accept
assert 'for(;;)' not in accept
assert accept.count('nova_accept4(listener,NOVA_SOCK_NONBLOCK)')==1
assert 'fd==-NOVA_EAGAIN || fd==-NOVA_EINTR' in accept
assert 'connected_count(sessions)>=NOVA_RESOURCE_BROKER_MAX_CLIENTS' in accept
limit=accept.index('connected_count(sessions)>=NOVA_RESOURCE_BROKER_MAX_CLIENTS')
lookup=accept.index('lookup_live_principal(peer_pid,&principal)')
assert limit<lookup,'capacity must short-circuit authority lookup'
assert re.search(r'if\(connected_count\(sessions\)>=NOVA_RESOURCE_BROKER_MAX_CLIENTS\)\s*\{\s*\(void\)nova_close\(\(int\)fd\);\s*continue;',accept), 'full-case must close fd'
assert 'peer_pid_for_fd((int)fd,&peer_pid)' in accept
assert 'g_auth_denials++' in accept
assert 'if(!placed) (void)nova_close((int)fd)' in accept
assert 'g_connections++' in accept

loop=func('nova_main')
assert 'if(fds[0].revents&NOVA_POLLIN) accept_sessions(listener,sessions);' in loop
assert 'process_session_packet(&sessions[i])' in loop
assert loop.index('accept_sessions(listener,sessions)') < loop.index('process_session_packet(&sessions[i])')
assert 'disconnect_session(sessions,i)' in loop
assert 'write_state(sessions,"LISTENING")' in loop
assert 'nova_listen((int)fd,(int)NOVA_RESOURCE_BROKER_MAX_CLIENTS)' in func('setup_listener')
assert 'nova_close(sessions[slot].fd)' in func('disconnect_session')
print('Phase-18 Step-6 R1 bounded accept/fair servicing source contract: PASS')
PYTHON

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
    "$B"
done
echo 'Phase-18 Step-6 R1 bounded accept dual-arch C syntax: PASS'
