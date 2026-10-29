#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1

"$ROOT/tests/run-stage5a-phase16-package-suite.sh"
"$ROOT/tests/stage5a-phase17-sandbox-foundation-source.sh"
"$ROOT/tests/stage5a-phase17-sandbox-foundation-model-unit.py"
"$ROOT/tests/stage5a-phase17-private-filesystem-source.sh"
"$ROOT/tests/stage5a-phase17-private-filesystem-model-unit.py"
"$ROOT/tests/stage5a-phase17-seccomp-ipc-source.sh"
"$ROOT/tests/stage5a-phase17-seccomp-ipc-model-unit.py"
"$ROOT/tests/stage5a-phase17-acceptance-probe-source.sh"
"$ROOT/tests/stage5a-phase17-harness-static-unit.py"
"$ROOT/tests/stage5a-phase17-acceptance-source.sh"
"$ROOT/tests/stage5a-phase17-app-socket-permission-source.sh"
"$ROOT/tests/stage5a-phase17-transport-diagnostics-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"

echo 'Stage-5A Phase-17 Step-4 kernel-enforced sandbox runtime-acceptance package suite: PASS'
