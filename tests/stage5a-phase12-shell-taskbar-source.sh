#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; C="$ROOT/src/services/compositor.c"; S="$ROOT/src/services/shell.c"; A="$ROOT/src/services/demo_app.c"
for marker in \
  'setup_desktop_listener' 'desktop_control=LISTENING protocol=1' 'desktop_task_hit' 'send_desktop_task_event' \
  'NOVA_DESKTOP_OP_TASK_EVENT' 'desktop_activate_client' 'desktop_close_client' 'desktop_launch_client' \
  'desktop_send_launch_event' 'primary_surface_for_client' 'desktop_control_connected=' \
  'desktop_app1_windows=' 'desktop_app2_windows=' \
  'draw_desktop_taskbar' 'task_base=NOVA_UI_TASK_BASE' 'task_stride=NOVA_UI_TASK_PITCH' 'launch_stride=70' 'display_name'; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-12 compositor marker: $marker" >&2; exit 1; }
done
for marker in \
  'NOVA_DESKTOP_SOCKET_PATH' 'respond_to_task_event' 'NOVA_DESKTOP_OP_ACTIVATE' 'NOVA_DESKTOP_OP_CLOSE' 'NOVA_DESKTOP_OP_LAUNCH' \
  'desktop_control=CONNECTED protocol=1 taskbar=dynamic-catalog-v1 launcher=dynamic-catalog-v1' \
  'task_activates=' 'task_closes=' 'task_launches='; do
  grep -Fq "$marker" "$S" || { echo "missing Phase-12 shell marker: $marker" >&2; exit 1; }
done
for marker in \
  'NOVA_SURFACE_EVENT_LAUNCH' 'open_window_set' 'desktop_launch=REOPEN client=' 'launch_events=' 'window_reopens=' \
  'g_app.next_buffer_id++'; do
  grep -Fq "$marker" "$A" || { echo "missing Phase-12 resident-app launcher marker: $marker" >&2; exit 1; }
done
# Policy boundary: panel click must emit an event to nova-shell; it must not call
# desktop_activate/close/launch directly inside the task hit branch.
python3 - "$C" <<'PY'
import sys
s=open(sys.argv[1]).read()
start=s.index('if (g_desktop.connected && desktop_task_hit')
end=s.index('} else {', start)
block=s[start:end]
assert 'send_desktop_task_event' in block
assert 'desktop_activate_client(' not in block
assert 'desktop_close_client(' not in block
assert 'desktop_launch_client(' not in block
PY
echo 'Stage-5A Phase-12 shell-policy taskbar/activate-close/resident-launch source contract: PASS'
