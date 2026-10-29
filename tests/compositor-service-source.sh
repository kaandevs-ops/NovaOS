#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
M="$ROOT/configs/services/compositor.service"
L="$ROOT/configs/services/services.list"
grep -qx 'name=compositor' "$M"
grep -qx 'exec=/System/Core/nova-compositor' "$M"
grep -qx 'restart=always' "$M"
grep -qx 'critical=no' "$M"
grep -qx 'enabled=yes' "$M"
grep -qx '/System/Services/compositor.service' "$L"
# Graphics is optional for boot authorization: failure must not weaken Stage-4D
# trust gates or make a display device required for Recovery/console operation.
! grep -Fq 'nova-compositor' "$ROOT/src/init/main.c"
echo 'Stage-5A compositor is an optional supervised service with console fallback preserved: PASS'
