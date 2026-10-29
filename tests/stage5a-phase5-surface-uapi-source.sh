#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/src/runtime/nova_surface_protocol.h"; C="$ROOT/src/services/compositor.c"
grep -Eq '#define NOVA_SURFACE_PROTOCOL_VERSION [23456]U' "$P"
grep -Fq '#define NOVA_SURFACE_MAX_CLIENT_SURFACES 8U' "$P"
for op in CREATE_SURFACE SET_POSITION ATTACH_BUFFER DAMAGE COMMIT HEARTBEAT; do grep -Fq "NOVA_SURFACE_OP_${op}" "$P"; done
for ev in FOCUS KEY POINTER; do grep -Fq "NOVA_SURFACE_EVENT_${ev}" "$P"; done
grep -Fq 'NOVA_SURFACE_SOCKET_PATH "/run/nova/client.sock"' "$P"
grep -Fq 'client_ipc=LISTENING protocol=' "$C"
grep -Fq 'client_ipc=CONNECTED protocol=' "$C"
echo 'Stage-5A Phase-5 client surface protocol compatibility contract: PASS'
