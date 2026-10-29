#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase11-package-suite.sh"
"$ROOT/tests/stage5a-phase12-desktop-control-uapi-source.sh"
"$ROOT/tests/stage5a-phase12-taskbar-model-unit.py"
"$ROOT/tests/stage5a-phase12-shell-taskbar-source.sh"
"$ROOT/tests/stage5a-phase12-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-12 reopened-package critical source suite: PASS'
