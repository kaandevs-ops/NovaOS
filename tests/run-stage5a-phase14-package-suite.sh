#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1

"$ROOT/tests/run-stage5a-phase13-package-suite.sh"
"$ROOT/tests/stage5a-phase14-app-catalog-uapi-source.sh"
"$ROOT/tests/stage5a-phase14-app-catalog-model-unit.py"
"$ROOT/tests/stage5a-phase14-novad-catalog-source.sh"
"$ROOT/tests/stage5a-phase14-catalog-producer-model-unit.py"
"$ROOT/tests/stage5a-phase14-shell-catalog-consumer-source.sh"
"$ROOT/tests/stage5a-phase14-shell-catalog-model-unit.py"
"$ROOT/tests/stage5a-phase14-dynamic-taskbar-source.sh"
"$ROOT/tests/stage5a-phase14-dynamic-taskbar-model-unit.py"
"$ROOT/tests/stage5a-phase14-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"

echo 'Stage-5A Phase-14 Step-5 runtime-acceptance harness package suite: PASS'
