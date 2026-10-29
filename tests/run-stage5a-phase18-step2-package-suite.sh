#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase18-package-suite.sh"
"$ROOT/tests/stage5a-phase18-step2-source.sh"
python3 "$ROOT/tests/stage5a-phase18-step2-grant-model-unit.py"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Phase-18 Step-2 R1 source/model/frozen cumulative package suite: PASS (not runtime acceptance)'
