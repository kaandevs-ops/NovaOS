#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
A="$ROOT/src/services/demo_app.c"
H="$ROOT/scripts/windows/stage5a-phase13-check.py"

for marker in \
  '#define APP_GRACEFUL_EXIT_LOG "[nova-demo-app-b] process_lifecycle=GRACEFUL_EXIT reason=all-toplevels-closed"' \
  '#define APP_STOPPED_LOG "[nova-demo-app-b] process_lifecycle=STOPPED exit=0"' \
  '#define APP_GRACEFUL_EXIT_LOG "[nova-demo-app] process_lifecycle=GRACEFUL_EXIT reason=all-toplevels-closed"' \
  '#define APP_STOPPED_LOG "[nova-demo-app] process_lifecycle=STOPPED exit=0"' \
  'nova_putln(APP_GRACEFUL_EXIT_LOG);' \
  'nova_putln(APP_STOPPED_LOG);'; do
  grep -Fq "$marker" "$A" || { echo "missing app lifecycle identity marker: $marker" >&2; exit 1; }
done

python3 - "$A" "$H" <<'PY_CHECK'
import re,sys
app=open(sys.argv[1],encoding='utf-8').read()
harness=open(sys.argv[2],encoding='utf-8').read()
b_grace='[nova-demo-app-b] process_lifecycle=GRACEFUL_EXIT reason=all-toplevels-closed'
b_stop='[nova-demo-app-b] process_lifecycle=STOPPED exit=0'
assert b_grace in app and b_stop in app
assert b_grace in harness and b_stop in harness
# Lifecycle call sites must use the compile-time identity macros rather than a hard-coded App-A prefix.
for literal in (
    'nova_putln("[nova-demo-app] process_lifecycle=GRACEFUL_EXIT reason=all-toplevels-closed")',
    'nova_putln("[nova-demo-app] process_lifecycle=STOPPED exit=0")',
):
    assert literal not in app
assert app.count('nova_putln(APP_GRACEFUL_EXIT_LOG);') == 1
assert app.count('nova_putln(APP_STOPPED_LOG);') == 1
PY_CHECK

clang -std=c11 -Wall -Wextra -Werror -fsyntax-only -I"$ROOT/src/runtime" "$A"
clang -std=c11 -Wall -Wextra -Werror -fsyntax-only -DNOVA_DEMO_APP_B -I"$ROOT/src/runtime" "$A"

echo 'Stage-5A Phase-13 App-A/App-B lifecycle log identity source contract: PASS'
