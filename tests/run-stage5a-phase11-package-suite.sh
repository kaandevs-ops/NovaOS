#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase10-package-suite.sh"
"$ROOT/tests/stage5a-phase11-multiclient-source.sh"
"$ROOT/tests/stage5a-phase11-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-11 reopened-package critical source suite: PASS'
