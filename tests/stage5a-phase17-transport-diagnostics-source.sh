#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"
P="$ROOT/tests/fixtures/stage5a-phase17/sandbox_probe.c"
H="$ROOT/scripts/windows/stage5a-phase17-check.py"

for marker in \
  'write_sandbox_ipc_state' \
  '/run/nova/sandbox-ipc.state' \
  'write_sandbox_ipc_state("LISTENING","none",(int)fd)' \
  'socket_mode=0666' \
  'transport_auth=SO_PEERCRED_NOVAD_PRINCIPAL' \
  'nova_listen((int)fd,16)'; do
  grep -Fq "$marker" "$C" || { echo "missing compositor transport diagnostic marker: $marker" >&2; exit 1; }
done

for marker in \
  'connect_attempts' \
  'connect_last_error' \
  'socket_last_error' \
  'NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK' \
  'write_state("CONNECTING")'; do
  grep -Fq "$marker" "$P" || { echo "missing probe transport diagnostic marker: $marker" >&2; exit 1; }
done

for marker in \
  "SANDBOX_IPC_STATE_PATH='/run/nova/sandbox-ipc.state'" \
  'app-facing compositor IPC listener' \
  'wait_probe_ready' \
  'connect_attempts=' \
  'socket_errno=' \
  'connect_errno='; do
  grep -Fq "$marker" "$H" || { echo "missing harness fail-fast transport marker: $marker" >&2; exit 1; }
done

python3 - "$H" <<'PY'
import pathlib,sys
p=pathlib.Path(sys.argv[1]); compile(p.read_text(encoding='utf-8'),str(p),'exec')
PY

echo 'Stage-5A Phase-17 app-facing IPC listener state + nonblocking errno telemetry fail-fast source contract: PASS'
