#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
M="$ROOT/configs/services/demo-app.service"; L="$ROOT/configs/services/services.list"; B="$ROOT/scripts/build-userland.sh"; I="$ROOT/scripts/build-initramfs.sh"; A="$ROOT/src/services/demo_app.c"
grep -qx 'name=demo-app' "$M"
grep -qx 'exec=/System/Apps/nova-demo-app' "$M"
grep -qx 'restart=always' "$M"
grep -qx 'critical=no' "$M"
grep -qx '/System/Services/demo-app.service' "$L"
grep -Fq 'src/services/demo_app.c' "$B"
grep -Fq 'System/Apps/nova-demo-app' "$B"
grep -Fq 'System/Apps/nova-demo-app' "$I"
grep -Fq 'demo-app.service' "$I"
for marker in NOVA_SURFACE_OP_CREATE_SURFACE NOVA_SURFACE_OP_SET_POSITION NOVA_SURFACE_OP_ATTACH_BUFFER NOVA_SURFACE_OP_DAMAGE NOVA_SURFACE_OP_COMMIT NOVA_SURFACE_EVENT_FOCUS NOVA_SURFACE_EVENT_KEY NOVA_SURFACE_EVENT_POINTER; do grep -Fq "$marker" "$A"; done
grep -Fq '[nova-demo-app] stage=CONNECTED protocol=' "$A"
grep -Fq 'NOVA_KEY_RIGHT' "$A"
echo 'Stage-5A Phase-5 supervised nova-demo-app + bidirectional surface-client compatibility contract: PASS'
