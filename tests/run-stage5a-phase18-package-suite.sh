#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1

"$ROOT/tests/run-stage5a-phase17-package-suite.sh"
"$ROOT/tests/stage5a-phase18-resource-broker-uapi-source.sh"
"$ROOT/tests/stage5a-phase18-resource-broker-principal-source.sh"
"$ROOT/tests/stage5a-phase18-resource-broker-model-unit.py"
"$ROOT/tests/package-hygiene.sh" "$ROOT"

echo 'Stage-5A Phase-18 Step-1 authenticated trusted resource-broker foundation package suite: PASS'
