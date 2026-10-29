#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase4-package-suite.sh"
"$ROOT/tests/stage5a-phase5-surface-uapi-source.sh"
"$ROOT/tests/stage5a-phase5-demo-app-source.sh"
"$ROOT/tests/stage5a-phase5-compositor-source.sh"
"$ROOT/tests/stage5a-phase5-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-5 reopened-package critical source suite: PASS'
