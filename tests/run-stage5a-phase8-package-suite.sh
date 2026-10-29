#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase7-package-suite.sh"
"$ROOT/tests/stage5a-phase8-font-runtime-unit.sh"
"$ROOT/tests/stage5a-phase8-font-source.sh"
"$ROOT/tests/stage5a-phase8-demo-app-source.sh"
"$ROOT/tests/stage5a-phase8-compositor-source.sh"
"$ROOT/tests/stage5a-phase8-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-8 reopened-package critical source suite: PASS'
