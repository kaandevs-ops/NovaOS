#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export PYTHONDONTWRITEBYTECODE=1

"$ROOT/tests/run-stage5a-phase12-package-suite.sh"

"$ROOT/tests/stage5a-phase13-app-lifecycle-uapi-source.sh"
"$ROOT/tests/stage5a-phase13-app-lifecycle-model-unit.py"
"$ROOT/tests/stage5a-phase13-app-registry-source.sh"
"$ROOT/tests/stage5a-phase13-app-registry-model-unit.py"
"$ROOT/tests/stage5a-phase13-novad-lifecycle-ipc-source.sh"
"$ROOT/tests/stage5a-phase13-shell-lifecycle-source.sh"
"$ROOT/tests/stage5a-phase13-shell-launch-policy-model-unit.py"
"$ROOT/tests/stage5a-phase13-graceful-stop-source.sh"
"$ROOT/tests/stage5a-phase13-graceful-stop-model-unit.py"
"$ROOT/tests/stage5a-phase13-cold-launch-source.sh"
"$ROOT/tests/stage5a-phase13-cold-launch-model-unit.py"
"$ROOT/tests/stage5a-phase13-app-state-source.sh"
"$ROOT/tests/stage5a-phase13-acceptance-source.sh"
"$ROOT/tests/stage5a-phase13-compositor-disconnect-state-source.sh"
"$ROOT/tests/stage5a-phase13-client-connected-state-source.sh"
"$ROOT/tests/stage5a-phase13-shell-poll-timeout-source.sh"
"$ROOT/tests/stage5a-phase13-app-lifecycle-log-identity-source.sh"

"$ROOT/tests/package-hygiene.sh" "$ROOT"

echo 'Stage-5A Phase-13 contract/model/registry/novad-IPC/shell-wiring/graceful-stop/cold-launch/runtime-harness package suite: PASS'
