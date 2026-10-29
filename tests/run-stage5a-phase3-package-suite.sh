#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-phase2-package-suite.sh"
"$ROOT/tests/stage5a-phase3-drm-uapi-source.sh"
"$ROOT/tests/stage5a-phase3-compositor-source.sh"
"$ROOT/tests/stage5a-phase3-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-3 reopened-package critical source suite: PASS'
