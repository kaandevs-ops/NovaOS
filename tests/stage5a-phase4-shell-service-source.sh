#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
M="$ROOT/configs/services/shell.service"; L="$ROOT/configs/services/services.list"; B="$ROOT/scripts/build-userland.sh"; I="$ROOT/scripts/build-initramfs.sh"; S="$ROOT/src/services/shell.c"
grep -qx 'name=shell' "$M"
grep -qx 'exec=/System/Core/nova-shell' "$M"
grep -qx 'restart=always' "$M"
grep -qx 'critical=no' "$M"
grep -qx '/System/Services/shell.service' "$L"
grep -Fq 'src/services/shell.c' "$B"
grep -Fq 'nova-shell' "$B"
grep -Fq 'System/Core/nova-shell' "$I"
grep -Fq 'shell.service' "$I"
grep -Fq 'NOVA_SHELL_OP_HELLO' "$S"
grep -Fq 'NOVA_SHELL_OP_SCENE' "$S"
grep -Fq '[nova-shell] stage=CONNECTED protocol=1' "$S"
echo 'Stage-5A Phase-4 nova-shell supervised service + compositor IPC producer contract: PASS'
