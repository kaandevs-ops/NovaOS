#!/usr/bin/env bash
set -uo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tests=(
  "$ROOT/scripts/verify-userland.sh"
  "$ROOT/tests/smoke-stage4a.sh"
  "$ROOT/tests/integrity-manifest.sh"
  "$ROOT/tests/signed-manifest.sh"
  "$ROOT/tests/signature-tamper.sh"
  "$ROOT/tests/trust-pin.sh"
  "$ROOT/tests/kernel-trust-config.sh"
  "$ROOT/tests/runtime-io.sh"
  "$ROOT/tests/logging-source.sh"
  "$ROOT/tests/reproducible-initramfs.sh"
  "$ROOT/tests/gpt-layout.sh"
  "$ROOT/tests/storage-source-stage4a.sh"
  "$ROOT/tests/metadata-journal-source.sh"
  "$ROOT/tests/clean-shutdown-source.sh"
  "$ROOT/tests/journal-fault-injection-source.sh"
  "$ROOT/tests/rollback-source.sh"
  "$ROOT/tests/slot-state-source.sh"
  "$ROOT/tests/verity-runtime-source.sh"
  "$ROOT/tests/verity-build-source.sh"
  "$ROOT/tests/preboot-verity-trust.sh"
  "$ROOT/tests/update-source-stage4a.sh"
  "$ROOT/tests/verity-rollback-source.sh"
  "$ROOT/tests/slot-confirm-health.sh"
  "$ROOT/tests/kernel-verity-runtime-config.sh"
  "$ROOT/tests/verity-uapi-abi.sh"
  "$ROOT/tests/verity-state-source.sh"
  "$ROOT/tests/recovery-verity-source.sh"
)
passed=0; failed=0; failed_names=()
for test_cmd in "${tests[@]}"; do
  echo; echo "=== RUN $(basename "$test_cmd") ==="
  if "$test_cmd"; then echo "=== PASS $(basename "$test_cmd") ==="; passed=$((passed+1));
  else echo "=== FAIL $(basename "$test_cmd") ===" >&2; failed=$((failed+1)); failed_names+=("$(basename "$test_cmd")"); fi
done
echo; echo "Stage-4A suite summary: passed=$passed failed=$failed"
if ((failed)); then printf 'Failed tests:' >&2; printf ' %s' "${failed_names[@]}" >&2; printf '\n' >&2; exit 1; fi
