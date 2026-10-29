#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHONDONTWRITEBYTECODE=1 bash "$ROOT/tests/stage5a-phase18-step7-r1-idle-source.sh"
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/tests/stage5a-phase18-step7-r1-idle-model.py"
PYTHONDONTWRITEBYTECODE=1 bash "$ROOT/tests/run-stage5a-phase18-step6-r2-package-suite.sh"
echo 'PHASE-18 STEP-7 R1 IDLE SESSION + FROZEN REGRESSIONS: PASS (not QEMU)'
