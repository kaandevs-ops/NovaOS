#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"; B="$ROOT/scripts/build-update-bundle.sh"
for needle in \
  '#define NOVA_UPDATE_PAYLOAD "/Data/Nova/Update/system.slot"' \
  'artifact_kind,"system-update"' \
  'install_update_slot_descriptor' \
  'signed verity descriptor install FAILED' \
  'copy_update_payload_to_device' \
  'nova_sha256_file(device,raw_hash)' \
  'one unconfirmed boot attempt is allowed'; do grep -Fq "$needle" "$SRC"; done
for needle in \
  'system.slot' 'format=2' 'artifact_kind=system-update' 'verity_root_hash=' 'verity_salt=' \
  'build-system-slot.sh' 'openssl dgst -sha256 -sign' 'openssl dgst -sha256 -verify'; do grep -Fq "$needle" "$B"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
start=s.index('if(nova_streq(args,"apply"))')
end=s.index('nova_putln("usage: update',start)
b=s[start:end]
assert b.index('copy_update_payload_to_device') < b.index('install_update_slot_descriptor') < b.index('write_slot_metadata_v2')
print('Update transaction ordering verified.')
PY
echo "Stage-3C signed verity update source test passed."
