#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"
SYS="$ROOT/src/runtime/nova_syscall.h"
for needle in \
  '#define NOVA_UPDATE_MANIFEST "/Data/Nova/Update/update.manifest"' \
  '#define NOVA_UPDATE_SIGNATURE "/Data/Nova/Update/update.sig"' \
  '#define NOVA_UPDATE_PAYLOAD "/Data/Nova/Update/system.ext4"' \
  'verify_signature_with_loaded_root' \
  'payload SHA-256 mismatch' \
  'copy_update_payload_to_device' \
  'inactive-slot write verification FAILED' \
  'one unconfirmed boot attempt is allowed'; do
  grep -Fq "$needle" "$SRC"
done
grep -Fq 'nova_open_wo' "$SYS"
grep -Fq 'nova_sha256_file(device,raw_hash)' "$SRC"
echo "Stage-3B signed inactive-slot update source test passed."
