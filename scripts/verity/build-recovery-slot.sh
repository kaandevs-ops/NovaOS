#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 6 ]]; then
  echo "usage: $0 <source-dir> <output-slot> <label> <uuid> <salt-hex> <metadata-out>" >&2
  exit 2
fi
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
exec "$ROOT/scripts/verity/build-verity-volume.sh" "$1" "$2" "$3" "$4" "$5" "$6" 20 24 Recovery
