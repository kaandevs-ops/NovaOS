#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"; BUILD="$ROOT/scripts/build-update-bundle.sh"; VALIDATOR="$ROOT/scripts/validate-release-policy.py"
for n in \
  'static int parse_update_manifest_v5' \
  'seen==((1UL<<19)-1UL)' \
  'else return 0;' \
  'if(seen&bit)return 0' \
  'NOVA_SYSTEM_SLOT_BYTES' \
  'NOVA_VERITY_DATA_BLOCKS' \
  'target_release_policy_sha256' \
  'verify_delegated_slot_descriptor' \
  'strict_verity.rollback_index!=desc->rollback_index' \
  'parse_unsigned_canonical(value,&v)'; do
  grep -Fq "$n" "$SRC"
done
for n in \
  'validate-release-policy.py' \
  '--expect-epoch "$TRUST_EPOCH"' \
  '--expect-active "$RELEASE_KEY_ID"' \
  'update-root-public-key.pem' \
  'release-policy.sig'; do
  grep -Fq -- "$n" "$BUILD"
done
for n in 'duplicate field' 'unknown field' 'value outside supported unsigned-long range'; do grep -Fq "$n" "$VALIDATOR"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
p=s[s.index('static int parse_update_manifest_v5'):s.index('static int verify_delegated_slot_descriptor')]
required=('format','artifact_kind','update_id','version','rollback_index','trust_epoch','signer_key_id','transition',
          'target_release_policy_sha256','payload','payload_sha256','payload_size','verity_data_block_size',
          'verity_hash_block_size','verity_data_blocks','verity_hash_start_block','verity_hash_algorithm',
          'verity_root_hash','verity_salt')
for key in required:
    assert f'key,"{key}"' in p, key
assert 'else return 0;' in p and 'if(seen&bit)return 0' in p
print('Stage-4D strict format-5 update manifest source contract passed.')
PY
