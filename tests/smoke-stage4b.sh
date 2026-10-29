#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NOVA_VERSION=0.0.9-dev "$ROOT/scripts/build-userland.sh" x86_64 >/dev/null
BIN="$ROOT/build/x86_64/rootfs/init"
OUT="$ROOT/build/x86_64/smoke-stage4b.txt"
printf 'version\nselftest\nboot\nfirmware\nlayout\nstorage\nverity\nslot\nupdate status\nupdate verify\ntrust\nintegrity\nsecurity\npersist show\necho nova-stage4b-test\npwd\nexit\n' | "$BIN" >"$OUT" 2>&1

grep -Fq 'NovaOS 0.0.9-dev [x86_64]' "$OUT"
grep -Fq 'SHA-256 self-test: PASS' "$OUT"
grep -Fq 'NovaOS Stage-4B firmware trust' "$OUT"
grep -Fq 'NovaOS Stage-4B storage policy' "$OUT"
grep -Fq 'NovaOS Stage-4B verified persistent storage' "$OUT"
grep -Fq 'NovaOS Stage-4B dm-verity state' "$OUT"
grep -Fq 'Recovery:' "$OUT"
grep -Fq 'NovaOS A/B slot foundation' "$OUT"
grep -Fq 'NovaOS Stage-4B signed update state' "$OUT"
grep -Fq 'update: persistent trusted system required' "$OUT"
grep -Fq 'NovaOS Stage-4B trust foundation' "$OUT"
grep -Fq 'NovaOS Stage-4B Security' "$OUT"
grep -Fq 'nova-stage4b-test' "$OUT"
echo "Stage-4B smoke test passed."
