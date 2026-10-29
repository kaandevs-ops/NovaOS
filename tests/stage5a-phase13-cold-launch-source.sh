#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
N="$ROOT/src/init/main.c"
S="$ROOT/src/services/shell.c"
C="$ROOT/src/services/compositor.c"
A="$ROOT/configs/services/demo-app.service"
B="$ROOT/configs/services/demo-app-b.service"

for manifest in "$A" "$B"; do
    grep -qx 'kind=application' "$manifest"
    grep -qx 'enabled=yes' "$manifest"
    grep -qx 'autostart=no' "$manifest"
    grep -qx 'restart=always' "$manifest"
done

for marker in \
  'g_services[i].enabled && g_services[i].autostart' \
  'application_launch_request' \
  'svc->desired_running=1' \
  'start_service(svc)' \
  'svc->app_generation++' \
  'svc->app_state=NOVA_APP_STATE_RUNNING'; do
    grep -Fq "$marker" "$N" || { echo "missing Phase-13 cold-launch novad marker: $marker" >&2; exit 1; }
done

for marker in \
  'NOVA_APP_OP_LAUNCH' \
  'launcher=NOVAD_PROCESS_SPAWN' \
  'NOVA_APP_RESULT_ALREADY_RUNNING' \
  'launcher=RESIDENT_WINDOW_FALLBACK'; do
    grep -Fq "$marker" "$S" || { echo "missing Phase-13 cold-launch shell marker: $marker" >&2; exit 1; }
done

for marker in \
  'draw_desktop_taskbar' \
  'client_surface_count_for(task->client_id)' \
  'NOVA_DESKTOP_ACTION_LAUNCH'; do
    grep -Fq "$marker" "$C" || { echo "missing Phase-13 cold-launch taskbar marker: $marker" >&2; exit 1; }
done

python3 - "$N" "$S" <<'PY'
import sys
novad=open(sys.argv[1],encoding='utf-8').read()
shell=open(sys.argv[2],encoding='utf-8').read()

# Boot supervision must respect autostart instead of starting every enabled entry.
start=novad.index('static void start_enabled_services')
end=novad.index('static void stop_service', start)
boot=novad[start:end]
assert 'g_services[i].enabled && g_services[i].autostart' in boot

# A successful lifecycle launch is a real novad spawn and must not also emit the
# resident-window launch command into the compositor.
start=shell.index('static int respond_to_task_event')
end=shell.index('static int connect_once', start)
block=shell[start:end]
launch=block[block.index('NOVA_DESKTOP_ACTION_LAUNCH'):]
assert 'lifecycle_launch(' in launch
assert 'status.result == NOVA_APP_RESULT_OK' in launch
assert 'launcher=NOVAD_PROCESS_SPAWN' in launch
ok_pos=launch.index('status.result == NOVA_APP_RESULT_OK')
fallback_pos=launch.index('status.result == NOVA_APP_RESULT_ALREADY_RUNNING')
assert ok_pos < fallback_pos
assert 'return 1;' in launch[ok_pos:fallback_pos]
PY

echo 'Stage-5A Phase-13 non-autostart cold-launch source contract: PASS'
