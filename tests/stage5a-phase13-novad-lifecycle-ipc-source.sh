#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/src/init/main.c"

for marker in \
  'setup_app_lifecycle_listener' \
  'NOVA_APP_SOCKET_PATH' \
  'NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK' \
  'nova_bind((int)fd,&addr,sizeof(addr))' \
  'nova_listen((int)fd,1)' \
  'process_app_lifecycle_packet' \
  'nova_recvmsg(fd,&msg,0)' \
  'NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC' \
  'app_request_header_valid' \
  'find_application_by_id(request.app_id)' \
  'application_launch_request' \
  'application_stop_request' \
  'NOVA_APP_RESULT_DENIED' \
  'NOVA_APP_RESULT_ALREADY_RUNNING' \
  'NOVA_APP_RESULT_NOT_RUNNING' \
  'NOVA_APP_STATE_STOPPING' \
  'NOVA_APP_STATE_RESTARTING' \
  'app_generation++' \
  'run_novad_event_loop' \
  'console_consume_char' \
  'reap_services()'
do
  grep -Fq "$marker" "$P" || { echo "missing Phase-13 novad lifecycle IPC marker: $marker" >&2; exit 1; }
done

python3 - "$P" <<'PY'
import re, sys
s=open(sys.argv[1],encoding='utf-8').read()
main=re.search(r'int nova_main\(void\) \{(.*)\n\}',s,re.S)
assert main
body=main.group(1)
assert 'run_novad_event_loop(app_listener' in body
assert 'nova_readline(' not in body, 'PID1 main loop must no longer block in nova_readline'
packet=re.search(r'static int process_app_lifecycle_packet\(int fd\) \{(.*?)\n\}',s,re.S)
assert packet
pb=packet.group(1)
for forbidden in ('nova_execve(', 'svc->exec', '/bin/sh'):
    assert forbidden not in pb, f'lifecycle packet parser must not execute transported content: {forbidden}'
assert 'find_application_by_id(request.app_id)' in pb

loop=re.search(r'static void run_novad_event_loop\([^)]*\) \{(.*?)\n\}',s,re.S)
assert loop
lb=loop.group(1)
# Phase-13 invariant: lifecycle listener/client remain nonblocking poll participants,
# even when later phases add additional event-loop descriptors.
for marker in (
    'fds[1].fd=app_listener',
    'fds[2].fd=app_client',
    'process_app_lifecycle_packet(app_client)',
    'accept_app_lifecycle_client(app_listener,&app_client)',
):
    assert marker in lb, marker
assert re.search(r'nova_poll\(fds,\s*[3-9][0-9]*\s*,&timeout\)',lb), 'event loop must poll lifecycle descriptors'
PY

clang -std=c11 -Wall -Wextra -Werror -fsyntax-only \
  -I"$ROOT/src/runtime" \
  -DNOVA_VERSION='"0.0.12-dev"' \
  -DNOVA_BUILD_ARCH='"x86_64"' \
  -DNOVA_ROOT_CERT_SHA256='"UNPINNED"' \
  -DNOVA_TRUST_PROFILE='"development"' \
  -DNOVA_TRUST_KEY_ID='"nova-development-root-2026"' \
  -DNOVA_UEFI_CERT_SHA256='"UNPINNED"' \
  -DNOVA_RELEASE_KEY_ID='"nova-development-release-2026-a"' \
  -DNOVA_TRUST_EPOCH=1 \
  "$ROOT/src/init/main.c"

echo 'Stage-5A Phase-13 novad nonblocking lifecycle IPC + console/reap event-loop source contract: PASS'
