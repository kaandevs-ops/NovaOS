#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1

"$ROOT/tests/run-stage5a-phase15-package-suite.sh"
"$ROOT/tests/stage5a-phase16-app-security-uapi-source.sh"
"$ROOT/tests/stage5a-phase16-app-security-model-unit.py"
"$ROOT/tests/stage5a-phase16-novad-capability-policy-source.sh"
"$ROOT/tests/stage5a-phase16-capability-policy-model-unit.py"
"$ROOT/tests/stage5a-phase16-peer-principal-source.sh"
"$ROOT/tests/stage5a-phase16-peer-principal-model-unit.py"
"$ROOT/tests/stage5a-phase16-capability-enforcement-source.sh"
"$ROOT/tests/stage5a-phase16-capability-enforcement-model-unit.py"
"$ROOT/tests/stage5a-phase16-acceptance-probe-source.sh"
"$ROOT/tests/stage5a-phase16-service-registry-capacity-source.sh"
"$ROOT/tests/stage5a-phase16-service-manifest-bounds-source.sh"
"$ROOT/tests/stage5a-phase16-service-manifest-bounds-model-unit.py"
"$ROOT/tests/stage5a-phase16-harness-static-unit.py"
"$ROOT/tests/stage5a-phase16-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"

echo 'Stage-5A Phase-16 Step-5 adversarial authenticated-principal/capability runtime-acceptance package suite: PASS'
