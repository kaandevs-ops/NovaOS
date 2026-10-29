#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NOVA_VERSION=0.0.7-dev "$ROOT/scripts/build-userland.sh" x86_64 >/dev/null
BIN="$ROOT/build/x86_64/rootfs/init"
OUT="$ROOT/build/x86_64/smoke-stage3c.txt"
printf 'version\nselftest\nboot\nlayout\nstorage\nverity\nslot\nupdate status\nupdate verify\ntrust\nintegrity\nsecurity\npersist show\necho nova-stage3c-test\npwd\nexit\n' | "$BIN" >"$OUT" 2>&1

grep -Fq 'NovaOS 0.0.7-dev [x86_64]' "$OUT"
grep -Fq 'SHA-256 self-test: PASS' "$OUT"
grep -Fq 'NovaOS Stage-3C storage policy' "$OUT"
grep -Fq 'NovaOS Stage-3C verified persistent storage' "$OUT"
grep -Fq 'NovaOS Stage-3C dm-verity state' "$OUT"
grep -Fq 'NovaOS A/B slot foundation' "$OUT"
grep -Fq 'NovaOS Stage-3C signed update state' "$OUT"
grep -Fq 'update: persistent trusted system required' "$OUT"
grep -Fq 'NovaOS Stage-3C trust foundation' "$OUT"
grep -Fq 'NovaOS Stage-3C Security' "$OUT"
grep -Fq 'nova-stage3c-test' "$OUT"
echo "Stage-3C smoke test passed."
