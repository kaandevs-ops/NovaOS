#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NOVA_VERSION=0.0.11 "$ROOT/scripts/build-userland.sh" x86_64 >/dev/null
BIN="$ROOT/build/x86_64/rootfs/init"
OUT="$ROOT/build/x86_64/smoke-stage4c.txt"
printf 'version\nselftest\nboot\nfirmware\ntpm\nevidence\nlayout\nstorage\nverity\nslot\nupdate status\nupdate verify\ntrust\nintegrity\nsecurity\npersist show\necho nova-stage4d-test\npwd\nexit\n' | "$BIN" >"$OUT" 2>&1

grep -Fq 'NovaOS 0.0.11 [x86_64]' "$OUT"
grep -Fq 'SHA-256 self-test: PASS' "$OUT"
grep -Fq 'NovaOS Stage-4D firmware trust' "$OUT"
grep -Fq 'NovaOS Stage-4D TPM / measured-boot state' "$OUT"
grep -Fq 'NovaOS Stage-4D measured-boot evidence' "$OUT"
grep -Fq 'NovaOS Stage-4D storage policy' "$OUT"
grep -Fq 'NovaOS Stage-4D verified persistent storage' "$OUT"
grep -Fq 'NovaOS Stage-4D dm-verity state' "$OUT"
grep -Fq 'Recovery:' "$OUT"
grep -Fq 'NovaOS A/B slot foundation' "$OUT"
grep -Fq 'NovaOS Stage-4D delegated-release update state' "$OUT"
grep -Fq 'update: persistent trusted system required' "$OUT"
grep -Fq 'NovaOS Stage-4D trust foundation' "$OUT"
grep -Fq 'NovaOS Stage-4D Security' "$OUT"
grep -Fq 'nova-stage4d-test' "$OUT"
echo "Stage-4C inherited smoke regression passed on Stage-4D userspace."
