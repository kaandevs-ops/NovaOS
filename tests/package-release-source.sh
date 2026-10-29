#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/scripts/release/package-stage4d-candidate.sh"
H="$ROOT/tests/package-hygiene.sh"
Q="$ROOT/tests/run-stage4d-package-suite.sh"
for needle in \
  "--exclude='./build'" "--exclude='./dist'" "--exclude='./.stage4d-backup'" "--exclude='./.git'" \
  'unzip -q "$ZIP"' 'run-stage4d-package-suite.sh' 'sha256sum "$ZIP"'; do
  grep -Fq -- "$needle" "$P" || { echo "missing package serialization/reopen contract: $needle" >&2; exit 1; }
done
for needle in 'dist/ must not be present' '.stage4d-backup/ must not be present' '.git/ metadata must not be present' 'PRIVATE KEY'; do
  grep -Fq "$needle" "$H" || { echo "missing package hygiene contract: $needle" >&2; exit 1; }
done
for needle in 'source-syntax.sh' 'release-identity-source.sh' 'boot-evidence-validator.py' 'measurement-transcript-runtime-unit.py' 'tcg2-eventlog-runtime.sh' 'rotation-negative-fixture-runtime-unit.sh' 'package-hygiene.sh'; do
  grep -Fq "$needle" "$Q" || { echo "missing reopened-package critical test: $needle" >&2; exit 1; }
done
echo 'Stage-4D candidate serialization/reopen/package-hygiene source contract passed.'
