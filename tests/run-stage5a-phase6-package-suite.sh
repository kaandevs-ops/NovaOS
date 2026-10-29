#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase5-package-suite.sh"
"$ROOT/tests/stage5a-phase6-linux-uapi-source.sh"
"$ROOT/tests/stage5a-phase6-buffer-uapi-source.sh"
"$ROOT/tests/stage5a-phase6-demo-app-source.sh"
"$ROOT/tests/stage5a-phase6-compositor-source.sh"
"$ROOT/tests/stage5a-phase6-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-6 reopened-package critical source suite: PASS'
