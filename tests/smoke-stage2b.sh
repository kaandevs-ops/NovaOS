#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT/scripts/build-userland.sh" x86_64 >/dev/null
BIN="$ROOT/build/x86_64/rootfs/init"
OUT="$ROOT/build/x86_64/smoke-stage2b.txt"
printf 'version\nselftest\nboot\nlayout\ntrust\nintegrity\nsecurity\necho nova-stage2b-test\npwd\nexit\n' | "$BIN" >"$OUT" 2>&1

grep -Fq 'NovaOS 0.0.4-dev [x86_64]' "$OUT"
grep -Fq 'SHA-256 self-test: PASS' "$OUT"
grep -Fq 'novad supervisor state' "$OUT"
grep -Fq 'NovaOS Stage-2B storage policy' "$OUT"
grep -Fq 'NovaOS Stage-2B trust foundation' "$OUT"
grep -Fq 'Kernel verify: Linux asymmetric-key/keyctl' "$OUT"
grep -Fq 'Firmware root: NOT CONNECTED YET' "$OUT"
grep -Fq 'NovaOS signed integrity foundation' "$OUT"
grep -Fq 'Firmware/UEFI verified boot  : NOT CONNECTED YET' "$OUT"
grep -Fq 'nova-stage2b-test' "$OUT"

echo "Stage-2B smoke test passed."
