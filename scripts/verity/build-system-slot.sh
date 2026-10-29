#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 6 ]]; then
  echo "usage: $0 <source-dir> <output-slot> <label> <uuid> <salt-hex> <metadata-out>" >&2
  exit 2
fi
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
case "${NOVA_R47_BROWSER_COMPAT:-0}" in
  0) DATA_MIB=40; SLOT_MIB=48 ;;
  1) DATA_MIB=512; SLOT_MIB=520 ;;
  *) echo "invalid R47 browser feature flag" >&2; exit 2 ;;
esac
exec "$ROOT/scripts/verity/build-verity-volume.sh" "$1" "$2" "$3" "$4" "$5" "$6" "$DATA_MIB" "$SLOT_MIB" System
