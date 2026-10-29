#!/usr/bin/env bash
set -uo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

tests=(
  "$ROOT/scripts/verify-userland.sh"
  "$ROOT/tests/smoke-stage2b.sh"
  "$ROOT/tests/integrity-manifest.sh"
  "$ROOT/tests/signed-manifest.sh"
  "$ROOT/tests/signature-tamper.sh"
  "$ROOT/tests/trust-pin.sh"
  "$ROOT/tests/kernel-trust-config.sh"
  "$ROOT/tests/runtime-io.sh"
  "$ROOT/tests/logging-source.sh"
  "$ROOT/tests/reproducible-initramfs.sh"
)

passed=0
failed=0
failed_names=()
for test_cmd in "${tests[@]}"; do
  echo
  echo "=== RUN $(basename "$test_cmd") ==="
  if "$test_cmd"; then
    echo "=== PASS $(basename "$test_cmd") ==="
    passed=$((passed+1))
  else
    echo "=== FAIL $(basename "$test_cmd") ===" >&2
    failed=$((failed+1))
    failed_names+=("$(basename "$test_cmd")")
  fi
done

echo
echo "Stage-2B suite summary: passed=$passed failed=$failed"
if ((failed)); then
  printf 'Failed tests:' >&2
  printf ' %s' "${failed_names[@]}" >&2
  printf '\n' >&2
  exit 1
fi
