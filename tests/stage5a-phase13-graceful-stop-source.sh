#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
N="$ROOT/src/init/main.c"
S="$ROOT/src/services/shell.c"
A="$ROOT/src/services/demo_app.c"
L="$ROOT/src/runtime/nova_linux.h"
Y="$ROOT/src/runtime/nova_syscall.h"

for marker in \
  'NOVA_CLOCK_MONOTONIC = 1' \
  'NOVA_NR_clock_gettime  228' \
  'NOVA_NR_clock_gettime  113'; do
  grep -Fq "$marker" "$L" || { echo "missing Phase-13 monotonic UAPI marker: $marker" >&2; exit 1; }
done
grep -Fq 'nova_clock_gettime' "$Y" || { echo 'missing nova_clock_gettime wrapper' >&2; exit 1; }

for marker in \
  'NOVA_APP_STOP_GRACE_NS 2000000000ULL' \
  'NOVA_APP_STOP_TERM_NS 1000000000ULL' \
  'monotonic_now_ns' \
  'application_stop_request' \
  'app_stop_graceful' \
  'enforce_application_stop_deadlines' \
  'app_stop_sigterm' \
  'app_stop_sigkill' \
  'NOVA_SIGTERM' \
  'NOVA_SIGKILL'; do
  grep -Fq "$marker" "$N" || { echo "missing Phase-13 graceful-stop novad marker: $marker" >&2; exit 1; }
done

for marker in \
  'lifecycle_stop' \
  'NOVA_APP_OP_STOP' \
  'NOVA_APP_STATE_STOPPING' \
  'app_stop_intents=' \
  'app_stop_commands=' \
  'close=graceful-stop-v1'; do
  grep -Fq "$marker" "$S" || { echo "missing Phase-13 graceful-stop shell marker: $marker" >&2; exit 1; }
done

for marker in \
  'process_lifecycle=GRACEFUL_EXIT reason=all-toplevels-closed' \
  'write_state("EXITING")' \
  'process_lifecycle=STOPPED exit=0'; do
  grep -Fq "$marker" "$A" || { echo "missing Phase-13 graceful-stop app marker: $marker" >&2; exit 1; }
done

python3 - "$N" "$S" "$A" <<'PY_CHECK'
import re,sys
novad,shell,app=(open(p,encoding='utf-8').read() for p in sys.argv[1:])

stop=re.search(r'static unsigned int application_stop_request\(struct nova_service \*svc\) \{(.*?)\n\}',novad,re.S)
assert stop
sb=stop.group(1)
assert 'svc->desired_running=0' in sb
assert 'NOVA_APP_STATE_STOPPING' in sb
assert 'NOVA_APP_STOP_GRACE_NS' in sb
assert 'nova_kill(' not in sb, 'STOP request must arm graceful shutdown, not kill immediately'

enforce=re.search(r'static void enforce_application_stop_deadlines\(void\) \{(.*?)\n\}',novad,re.S)
assert enforce
eb=enforce.group(1)
assert 'NOVA_SIGTERM' in eb and 'NOVA_SIGKILL' in eb
assert eb.index('NOVA_SIGTERM') < eb.index('NOVA_SIGKILL')

respond=shell[shell.index('static int respond_to_task_event'):shell.index('static int connect_once')]
close=respond[respond.index('NOVA_DESKTOP_ACTION_CLOSE'):respond.index('NOVA_DESKTOP_ACTION_LAUNCH')]
assert 'lifecycle_stop(' in close
assert 'NOVA_APP_STATE_STOPPING' in close
assert 'NOVA_DESKTOP_OP_CLOSE' in close
assert close.index('lifecycle_stop(') < close.index('NOVA_DESKTOP_OP_CLOSE')

close_event=app[app.index('ev.type==NOVA_SURFACE_EVENT_CLOSE_REQUEST'):]
assert 'g_app.surface_a_closed && g_app.surface_b_closed' in close_event
assert 'return 1;' in close_event
main=app[app.index('int nova_main(void)'):]
assert 'graceful_exit=run_connection' in main
assert 'if (graceful_exit)' in main
assert 'return 0;' in main
PY_CHECK

clang -std=c11 -Wall -Wextra -Werror -fsyntax-only \
  -I"$ROOT/src/runtime" "$S" "$A"

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
  "$N"

echo 'Stage-5A Phase-13 graceful STOP intent + compositor close + bounded TERM/KILL escalation source contract: PASS'
