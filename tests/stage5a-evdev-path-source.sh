#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C="$ROOT/src/services/compositor.c"
grep -Fq 'Linux evdev nodes are event0..event31, never zero-padded event00.' "$C"
grep -Fq 'input_event_path(path, i);' "$C"
! grep -Fq '"/dev/input/event00"' "$C"
grep -Fq "path[17] = '\\0';" "$C"
grep -Fq "path[18] = '\\0';" "$C"
echo 'Stage-5A evdev node naming regression contract: PASS'
