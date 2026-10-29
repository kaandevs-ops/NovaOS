#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAIN="$ROOT/src/init/main.c"
INITRAMFS="$ROOT/scripts/build-initramfs.sh"
LIST="$ROOT/configs/services/services.list"

capacity="$(sed -nE 's/^#define MAX_SERVICES ([0-9]+)$/\1/p' "$MAIN")"
[[ "$capacity" =~ ^[0-9]+$ ]] || { echo 'FAIL: MAX_SERVICES not found'; exit 1; }
base_count="$(grep -vcE '^[[:space:]]*(#|$)' "$LIST")"
probe_count="$(sed -n '/PHASE16PROBES/,/PHASE16PROBES/p' "$INITRAMFS" | grep -c '^/System/Services/phase16-security-probe-.*\.service$')"
required=$((base_count + probe_count))

(( capacity >= required )) || {
  echo "FAIL: service registry capacity $capacity < production+acceptance requirement $required"
  exit 1
}
(( capacity <= 64 )) || { echo 'FAIL: service registry bound unexpectedly large'; exit 1; }

grep -Fq 'int registry_overflow=0;' "$MAIN"
grep -Fq 'while (*p) {' "$MAIN"
grep -Fq 'if (g_service_count>=MAX_SERVICES) {' "$MAIN"
grep -Fq 'registry_overflow=1;' "$MAIN"
grep -Fq 'if (registry_overflow) {' "$MAIN"
grep -Fq 'g_service_count=0;' "$MAIN"
grep -Fq 'registry_capacity_exceeded' "$MAIN"
grep -Fq 'all services blocked' "$MAIN"

if grep -Fq 'while (*p && g_service_count<MAX_SERVICES)' "$MAIN"; then
  echo 'FAIL: silent service-list truncation loop remains'
  exit 1
fi

printf 'Stage-5A Phase-16 bounded service-registry capacity + overflow fail-closed source contract: PASS\n'
