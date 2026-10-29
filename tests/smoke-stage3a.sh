#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT/scripts/build-userland.sh" x86_64 >/dev/null
BIN="$ROOT/build/x86_64/rootfs/init"
OUT="$ROOT/build/x86_64/smoke-stage3a.txt"
printf 'version\nselftest\nboot\nlayout\nstorage\nslot\ntrust\nintegrity\nsecurity\npersist show\necho nova-stage3a-test\npwd\nexit\n' | "$BIN" >"$OUT" 2>&1

grep -Fq 'NovaOS 0.0.5-dev [x86_64]' "$OUT"
grep -Fq 'SHA-256 self-test: PASS' "$OUT"
grep -Fq 'NovaOS Stage-3A storage policy' "$OUT"
grep -Fq 'NovaOS Stage-3A persistent storage' "$OUT"
grep -Fq 'NovaOS A/B slot foundation' "$OUT"
grep -Fq 'Metadata trust: UNAUTHENTICATED (Stage-3A foundation)' "$OUT"
grep -Fq 'NovaOS Stage-3A trust foundation' "$OUT"
grep -Fq 'NovaOS Stage-3A Security' "$OUT"
grep -Fq 'persist: persistent Data is unavailable' "$OUT"
grep -Fq 'nova-stage3a-test' "$OUT"
echo "Stage-3A smoke test passed."
