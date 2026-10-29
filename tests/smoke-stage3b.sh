#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT/scripts/build-userland.sh" x86_64 >/dev/null
BIN="$ROOT/build/x86_64/rootfs/init"
OUT="$ROOT/build/x86_64/smoke-stage3b.txt"
printf 'version\nselftest\nboot\nlayout\nstorage\nslot\nupdate status\nupdate verify\ntrust\nintegrity\nsecurity\npersist show\necho nova-stage3b-test\npwd\nexit\n' | "$BIN" >"$OUT" 2>&1

grep -Fq 'NovaOS 0.0.6-dev [x86_64]' "$OUT"
grep -Fq 'SHA-256 self-test: PASS' "$OUT"
grep -Fq 'NovaOS Stage-3B storage policy' "$OUT"
grep -Fq 'NovaOS Stage-3B persistent storage' "$OUT"
grep -Fq 'NovaOS A/B slot foundation' "$OUT"
grep -Fq 'Automatic A/B rollback' "$OUT"
grep -Fq 'NovaOS Stage-3B signed update state' "$OUT"
grep -Fq 'update: persistent trusted system required' "$OUT"
grep -Fq 'NovaOS Stage-3B trust foundation' "$OUT"
grep -Fq 'NovaOS Stage-3B Security' "$OUT"
grep -Fq 'nova-stage3b-test' "$OUT"
echo "Stage-3B smoke test passed."
