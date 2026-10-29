#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1
# Run only against a fresh source tree (no build/ yet). Once built, use a
# separate source copy or the existing R3 hygiene-safe harness mechanism.
bash "$ROOT/tests/run-stage5a-phase18-step2-package-suite.sh"
bash "$ROOT/tests/stage5a-phase18-step3-fd-type-source.sh"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo 'Phase-18 Step-3 R1 cumulative source/model/frozen package suite: PASS (not guest acceptance)'
