#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
"$ROOT/tests/run-stage4d-package-suite.sh"
"$ROOT/tests/kernel-graphics-config.sh"
"$ROOT/tests/graphics-runtime-source.sh"
"$ROOT/tests/compositor-service-source.sh"
"$ROOT/tests/stage5a-acceptance-source.sh"
"$ROOT/tests/stage5a-evdev-path-source.sh"
"$ROOT/tests/stage5a-swtpm-socket-path-source.sh"
"$ROOT/tests/stage5a-compositor-readiness-wait-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Stage-5A Phase-1 reopened-package critical source suite: PASS'
