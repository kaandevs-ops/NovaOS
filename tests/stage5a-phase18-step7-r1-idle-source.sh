#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
B="$ROOT/src/services/resource_broker.c"
H="$ROOT/src/runtime/nova_resource_broker_protocol.h"
[[ -f "$B" && -f "$H" ]]
python3 - "$B" "$H" <<'PY'
from pathlib import Path
import re, sys
s=Path(sys.argv[1]).read_text(); h=Path(sys.argv[2]).read_text()
assert re.search(r'#define\s+NOVA_RESOURCE_BROKER_MAX_CLIENTS\s+4U\b',h)
assert 'NOVA_RESOURCE_BROKER_IDLE_SECONDS 120ULL' in s
assert 'last_activity_sec' in s
assert 'nova_clock_gettime(NOVA_CLOCK_MONOTONIC,&now)' in s
assert 'g_idle_expirations++' in s
assert 'idle_timeout_seconds=120' in s
assert 'idle_expirations=' in s
assert 'clock_failures=' in s

def func(name):
    m=re.search(r'\b'+re.escape(name)+r'\s*\([^;]*?\)\s*\{',s,re.S)
    assert m,name
    i=m.end(); depth=1
    while i<len(s) and depth:
        if s[i]=='{':depth+=1
        elif s[i]=='}':depth-=1
        i+=1
    assert depth==0,name
    return s[m.start():i]

req=func('process_session_packet')
assert req.index('!request_valid(&request)') < req.index('session->last_activity_sec=(unsigned long long)activity_sec')
assert req.index('session->last_activity_sec=(unsigned long long)activity_sec') < req.index('request.opcode==NOVA_RESOURCE_BROKER_OP_OPEN_DEMO_RO')
assert 'if(activity_sec<0){g_clock_failures++;return -1;}' in req
accept=func('accept_sessions')
assert 'for(unsigned int accepted=0U;accepted<NOVA_RESOURCE_BROKER_MAX_CLIENTS;accepted++)' in accept
assert accept.index('connected_count(sessions)>=NOVA_RESOURCE_BROKER_MAX_CLIENTS') < accept.index('lookup_live_principal(peer_pid,&principal)')
assert accept.index('lookup_live_principal(peer_pid,&principal)') < accept.index('long connected_sec=broker_now_seconds()')
assert 'sessions[i].last_activity_sec=(unsigned long long)connected_sec' in accept
assert 'if(connected_sec<0)' in accept and 'nova_close((int)fd)' in accept
expire=func('expire_idle_sessions')
assert 'if(now<0)' in expire and 'disconnect_session(sessions,i)' in expire
assert '(unsigned long long)now-last>=NOVA_RESOURCE_BROKER_IDLE_SECONDS' in expire
assert expire.index('disconnect_session(sessions,i)') < expire.index('g_idle_expirations++')
loop=func('nova_main')
assert loop.index('accept_sessions(listener,sessions)') < loop.index('process_session_packet(&sessions[i])')
assert loop.index('process_session_packet(&sessions[i])') < loop.index('expire_idle_sessions(sessions)')
assert loop.index('expire_idle_sessions(sessions)') < loop.rindex('write_state(sessions,"LISTENING")')
assert 'nova_listen((int)fd,(int)NOVA_RESOURCE_BROKER_MAX_CLIENTS)' in func('setup_listener')
print('Phase-18 Step-7 R1 monotonic idle expiry source contract: PASS')
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
    "$B"
done
echo 'Phase-18 Step-7 R1 idle broker x86_64/ARM64 syntax: PASS'
