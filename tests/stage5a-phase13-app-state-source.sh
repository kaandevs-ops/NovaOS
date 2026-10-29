#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
F="$ROOT/src/init/main.c"
for marker in \
  '#define NOVA_APP_STATE_FILE "/run/nova/app-lifecycle.state"' \
  'static void write_app_lifecycle_state(void)' \
  'schema=1' 'registry_count=' '_client_id=' '_state=' '_pid=' '_generation=' \
  '_desired_running=' '_restart_count=' '_autostart=' '_stop_signal_stage=' \
  'if(svc->is_application) write_app_lifecycle_state();' \
  'write_app_lifecycle_state();' ; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-13 lifecycle-state marker: $marker" >&2; exit 1; }
done
python3 - "$F" <<'PY'
import sys
s=open(sys.argv[1],encoding='utf-8').read()
assert s.count('NOVA_APP_STATE_FILE') >= 2
assert 'start_enabled_services();\n        write_app_lifecycle_state();' in s
assert 'log_service("INFO","app_stop_graceful",svc,-1);\n    write_app_lifecycle_state();' in s
assert 'log_service("INFO","started",svc,-1);\n    if(svc->is_application) write_app_lifecycle_state();' in s
PY
CC="${CC:-clang}"
"$CC" -std=c11 -Wall -Wextra -Werror -fsyntax-only \
  -DNOVA_VERSION='"0.0.12-dev"' \
  -DNOVA_ARCH='"x86_64"' \
  -I"$ROOT/src/runtime" "$ROOT/src/init/main.c"
echo 'Stage-5A Phase-13 machine-readable application lifecycle state source contract: PASS'
