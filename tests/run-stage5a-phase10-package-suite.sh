#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase9-package-suite.sh"
"$ROOT/tests/stage5a-phase10-selection-clipboard-runtime-unit.sh"
"$ROOT/tests/stage5a-phase10-clipboard-uapi-source.sh"
"$ROOT/tests/stage5a-phase10-demo-app-source.sh"
"$ROOT/tests/stage5a-phase10-compositor-source.sh"
"$ROOT/tests/stage5a-phase10-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-10 reopened-package critical source suite: PASS'
