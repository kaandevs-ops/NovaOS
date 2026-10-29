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
for needle in 'system.slot' 'format=5' 'artifact_kind=system-update' 'rollback_index=$ROLLBACK_INDEX' 'trust_epoch=$TRUST_EPOCH' 'signer_key_id=$RELEASE_KEY_ID' 'transition=$TRANSITION' 'target_release_policy_sha256=$TARGET_POLICY_SHA256' 'release-policy.conf' 'release-policy.sig' 'release-active.der' 'verity_root_hash=' 'verity_salt=' 'build-system-slot.sh' 'openssl dgst -sha256 -sign' 'openssl dgst -sha256 -verify'; do grep -Fq "$needle" "$B"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text(); start=s.index('if(nova_streq(args,"apply"))'); end=s.index('nova_putln("usage: update',start); b=s[start:end]
assert b.index('copy_update_payload_to_device') < b.index('install_update_slot_descriptor') < b.index('commit_slot_metadata')
install=s[s.index('static int install_update_slot_descriptor'):s.index('static int copy_update_payload_to_device')]
assert install.index('copy_file_atomic_path(NOVA_UPDATE_RELEASE_POLICY') < install.index('copy_file_atomic_path(NOVA_UPDATE_MANIFEST')
assert install.index('copy_file_atomic_path(NOVA_UPDATE_RELEASE_CERT') < install.index('copy_file_atomic_path(NOVA_UPDATE_MANIFEST')
print('Stage-4C/4D update transaction ordering verified.')
PY
echo "Stage-4C signed verity update source test passed."
