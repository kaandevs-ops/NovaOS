#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase8-package-suite.sh"
"$ROOT/tests/stage5a-phase9-widget-text-runtime-unit.sh"
python3 "$ROOT/tests/stage5a-phase9-frame-sync-unit.py"
"$ROOT/tests/stage5a-phase9-input-uapi-source.sh"
"$ROOT/tests/stage5a-phase9-demo-app-source.sh"
"$ROOT/tests/stage5a-phase9-compositor-source.sh"
"$ROOT/tests/stage5a-phase9-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-9 reopened-package critical source suite: PASS'
