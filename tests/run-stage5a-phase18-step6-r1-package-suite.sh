#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHONDONTWRITEBYTECODE=1 bash "$ROOT/tests/stage5a-phase18-step6-r1-accept-budget-source.sh"
PYTHONDONTWRITEBYTECODE=1 python3 "$ROOT/tests/stage5a-phase18-step6-r1-accept-budget-model.py"
PYTHONDONTWRITEBYTECODE=1 bash "$ROOT/tests/run-stage5a-phase18-step3-r2-safe-suite.sh"
echo 'PHASE-18 STEP-6 R1 BOUNDED ACCEPT + FROZEN REGRESSIONS: PASS (no QEMU yet)'
