#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INIT="$ROOT/scripts/build-initramfs.sh"; UPD="$ROOT/scripts/build-update-bundle.sh"; POLICY_VALIDATOR="$ROOT/scripts/validate-release-policy.py"; SRC="$ROOT/src/init/main.c"; TPM="$ROOT/src/runtime/nova_tpm2.c"; H="$ROOT/src/runtime/nova_tpm2.h"
for n in 'NOVA_RELEASE_KEY_ID' 'NOVA_RELEASE_NEXT_KEY_ID' 'NOVA_RELEASE_NEXT_CERT' 'NOVA_RELEASE_REVOKED_KEY_IDS'; do grep -Fq "$n" "$INIT"; done
for n in 'NOVA_UPDATE_SIGNER_KEY_ID' 'NOVA_UPDATE_TRUST_EPOCH' 'NOVA_UPDATE_TRANSITION' 'NOVA_UPDATE_SIGNING_KEY' 'target_release_policy_sha256=' 'NOVA_PREVIOUS_RELEASE_POLICY' 'validate-release-policy.py'; do grep -Fq "$n" "$UPD"; done
for n in 'promote-next target policy attempted to un-revoke' 'promote-next target active certificate is not the exact previous next certificate' 'previous trust_epoch exhausted' 'duplicate field' 'unknown field'; do grep -Fq "$n" "$POLICY_VALIDATOR"; done
for n in 'NOVA_TPM2_NV_TRUST_EPOCH_INDEX    0x01504e58UL' 'NOVA_TPM2_NV_TRUST_BASE_INDEX     0x01504e59UL' 'nova_tpm2_increment_trust_epoch_counter'; do grep -Fq "$n" "$H" "$TPM"; done
for n in \
 'Next release key must differ from active key' 'Next release key may not already be revoked' \
 'release_id_revoked(desc->signer_key_id)' 'checked_add_ul(g_release_trust_epoch,1UL,&next_epoch)' \
 'rotation target active certificate is not the exact previously delegated next key' \
 'g_release_next_key' 'verify_delegated_slot_descriptor' 'g_preboot_release_policy_verified' \
 'install_update_slot_descriptor(target,&desc)' 'nova_tpm2_increment_trust_epoch_counter' \
 'trust_epoch_commit_recovered'; do grep -Fq "$n" "$INIT" "$SRC"; done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
v=s[s.index('static int verify_update_bundle'):s.index('static int copy_file_atomic')]
assert v.index('release_id_revoked(desc->signer_key_id)') < v.index('unknown release signer rejected')
assert 'checked_add_ul(g_release_trust_epoch,1UL,&next_epoch)' in v
assert 'desc->trust_epoch!=next_epoch' in v
assert 'nova_streq(target.active_cert_sha256,g_release_next_cert_sha256)' in v
c=s[s.index('if(nova_streq(args,"confirm"))'):s.index('if(nova_startswith(args,"set "))')]
assert c.index('commit_slot_metadata') < c.index('nova_tpm2_increment_rollback_counter')
assert c.index('nova_tpm2_increment_rollback_counter') < c.index('nova_tpm2_increment_trust_epoch_counter')
r=s[s.index('static int recover_tpm_counter_commit_if_needed'):s.index('static int perform_measured_boot')]
assert 'nova_tpm2_increment_rollback_counter' in r and 'nova_tpm2_increment_trust_epoch_counter' in r
print('Stage-4D active->next rotation + dual TPM commit/recovery source contract passed.')
PY

# Revocations are monotonic across an authorized promote-next transition.
grep -Fq 'release_revocation_superset(target.revoked_key_ids,g_release_revoked_key_ids)' "$SRC"
grep -Fq 'rotation target attempted to remove a previously revoked release key' "$SRC"
