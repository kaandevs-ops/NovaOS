#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/src/runtime/nova_desktop_protocol.h"
S="$ROOT/src/services/shell.c"
C="$ROOT/src/services/compositor.c"

for marker in \
  '#define NOVA_DESKTOP_PROTOCOL_VERSION 1U' \
  '#define NOVA_DESKTOP_TASK_MAX_ENTRIES 64U' \
  'NOVA_DESKTOP_OP_TASK_MODEL_RESET' \
  'NOVA_DESKTOP_OP_TASK_MODEL_DEFINE' \
  'NOVA_DESKTOP_OP_TASK_MODEL_COMMIT' \
  'NOVA_DESKTOP_TASK_FLAG_LAUNCHABLE' \
  'sizeof(struct nova_desktop_message) == 48' \
  'sizeof(struct nova_desktop_task_message) == 112'; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-14 desktop task-model UAPI marker: $marker" >&2; exit 1; }
done

for marker in \
  'sync_desktop_task_model' \
  'NOVA_DESKTOP_OP_TASK_MODEL_RESET' \
  'NOVA_DESKTOP_OP_TASK_MODEL_DEFINE' \
  'NOVA_DESKTOP_OP_TASK_MODEL_COMMIT' \
  'entry->display_name' \
  'taskbar=dynamic-catalog-v1 launcher=dynamic-catalog-v1' \
  'desktop_task_model=SYNCED'; do
  grep -Fq "$marker" "$S" || { echo "missing Phase-14 shell dynamic-task marker: $marker" >&2; exit 1; }
done

for forbidden in 'SHELL_APP_A_ID' 'SHELL_APP_B_ID' 'app_id_for_client'; do
  ! grep -Fq "$forbidden" "$S" || { echo "Phase-14 shell still contains hard-coded application mapping: $forbidden" >&2; exit 1; }
done

for marker in \
  'struct nova_desktop_task_model' \
  'consume_desktop_task_message' \
  'desktop_task_for_client' \
  'desktop_task_model=COMMITTED' \
  'client_not_in_task_model' \
  'task_base=NOVA_UI_TASK_BASE' \
  'task_stride=NOVA_UI_TASK_PITCH' \
  'launch_stride=70' \
  't->display_name' \
  'nova_recvmsg(client,&msg,0)' \
  'NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC'; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-14 compositor dynamic-task marker: $marker" >&2; exit 1; }
done

! grep -Fq 'm.client_id<1U || m.client_id>2U' "$C" || { echo 'fixed client-id desktop policy survived Phase-14' >&2; exit 1; }
! grep -Fq 'if (x>=220 && x<300)' "$C" || { echo 'fixed App-A task hit region survived Phase-14' >&2; exit 1; }
! grep -Fq 'else if (x>=330 && x<410)' "$C" || { echo 'fixed App-B task hit region survived Phase-14' >&2; exit 1; }

python3 - "$C" <<'INNER_PY'
import sys
s=open(sys.argv[1],encoding='utf-8').read()
start=s.index('static int consume_desktop_task_message')
end=s.index('static int send_desktop_task_event', start)
block=s[start:end]
for marker in ('staging_entries','staging_expected','staging_generation','desktop_staging_has_client','NOVA_DESKTOP_OP_TASK_MODEL_COMMIT'):
    assert marker in block
assert block.index('staging_entries') < block.index('g_desktop_tasks.entries')
assert 'g_scene.visual_generation++' in block
INNER_PY

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat > "$TMP/check.c" <<'INNER_C'
#include "nova_desktop_protocol.h"
int main(void) { return sizeof(struct nova_desktop_message)==48 && sizeof(struct nova_desktop_task_message)==112 ? 0 : 1; }
INNER_C
clang -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/runtime" -fsyntax-only "$TMP/check.c"

echo 'Stage-5A Phase-14 additive desktop task-model + dynamic compositor taskbar source contract: PASS'
