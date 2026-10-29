#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage5a-package-suite.sh"
"$ROOT/tests/stage5a-phase2-drm-uapi-source.sh"
"$ROOT/tests/stage5a-phase2-compositor-source.sh"
"$ROOT/tests/stage5a-phase2-kms-ownership-source.sh"
"$ROOT/tests/stage5a-phase2-acceptance-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-2 reopened-package critical source suite: PASS'
