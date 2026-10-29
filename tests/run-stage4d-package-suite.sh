#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PYTHONDONTWRITEBYTECODE=1

"$ROOT/tests/package-hygiene.sh" "$ROOT"
"$ROOT/tests/source-syntax.sh"
"$ROOT/tests/release-identity-source.sh"
"$ROOT/tests/stage4d-failure-model.sh"
"$ROOT/tests/update-manifest-v5-source.sh"
"$ROOT/tests/evidence-state-source.sh"
python3 "$ROOT/tests/boot-evidence-validator.py"
python3 "$ROOT/tests/measurement-transcript-runtime-unit.py"
"$ROOT/tests/release-policy-source.sh"
"$ROOT/tests/release-key-reuse-source.sh"
"$ROOT/tests/release-rotation-source.sh"
"$ROOT/tests/uefi-key-lifecycle-source.sh"
"$ROOT/tests/rotation-fixture-runtime-unit.sh"
"$ROOT/tests/rotation-negative-fixture-runtime-unit.sh"
"$ROOT/tests/stage4d-acceptance-source.sh"
"$ROOT/tests/tcg2-eventlog-runtime.sh"
rm -rf "$ROOT/build"
"$ROOT/tests/package-hygiene.sh" "$ROOT"
echo "Stage-4D reopened-package critical source suite: PASS"
