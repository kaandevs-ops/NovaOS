#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1

"$ROOT/tests/run-stage5a-phase14-package-suite.sh"
"$ROOT/tests/stage5a-phase15-app-metadata-uapi-source.sh"
"$ROOT/tests/stage5a-phase15-app-metadata-model-unit.py"
"$ROOT/tests/stage5a-phase15-novad-metadata-source.sh"
"$ROOT/tests/stage5a-phase15-metadata-producer-model-unit.py"
"$ROOT/tests/stage5a-phase15-shell-metadata-consumer-source.sh"
"$ROOT/tests/stage5a-phase15-shell-metadata-cache-model-unit.py"
"$ROOT/tests/package-hygiene.sh" "$ROOT"

"$ROOT/tests/stage5a-phase15-asset-uapi-source.sh"
"$ROOT/tests/stage5a-phase15-asset-registry-source.sh"
"$ROOT/tests/stage5a-phase15-asset-registry-model-unit.py"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-15 Step-4 trusted asset registry/icon-resource foundation package suite: PASS'

"$ROOT/tests/stage5a-phase15-rich-launcher-source.sh"
"$ROOT/tests/stage5a-phase15-rich-launcher-model-unit.py"
"$ROOT/tests/stage5a-phase15-harness-static-unit.py"
"$ROOT/tests/stage5a-phase15-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-15 Step-5 rich launcher/trusted-icon runtime-acceptance package suite: PASS'
