#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase3-package-suite.sh"
"$ROOT/tests/stage5a-phase4-ipc-uapi-source.sh"
"$ROOT/tests/stage5a-phase4-shell-service-source.sh"
"$ROOT/tests/stage5a-phase4-scene-source.sh"
"$ROOT/tests/stage5a-phase4-presented-scene-source.sh"
python3 "$ROOT/tests/stage5a-phase4-state-parser-unit.py"
"$ROOT/tests/stage5a-phase4-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-4 reopened-package critical source suite: PASS'
