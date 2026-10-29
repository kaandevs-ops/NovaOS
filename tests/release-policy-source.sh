#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INIT="$ROOT/scripts/build-initramfs.sh"; UPD="$ROOT/scripts/build-update-bundle.sh"; SRC="$ROOT/src/init/main.c"
for n in \
  'release-policy.conf' 'release-policy.sig' 'release-active.der' 'release-next.der' \
  'format=3' 'policy_id=novaos-release-policy' 'policy_state=$POLICY_STATE' 'signature_algorithm=rsa-pkcs1-sha256' 'trust_epoch=' 'active_key_id=' 'active_cert_sha256=' \
  'next_key_id=' 'next_cert=' 'next_cert_sha256=' 'revoked_key_ids=' \
  'policy_binding=root-detached-signature+root-signed-system-manifest+dm-verity+TPM-trust-epoch' \
  'validate_revocation_ids' 'cert_spki_sha256' 'content-root public key must not reuse a UEFI PK/KEK/db public key' \
  'delegated release public key must not reuse the NovaOS content-root public key' \
  'active and next delegated release public keys must be distinct'; do grep -Fq "$n" "$INIT"; done
python3 - "$INIT" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
assert s.index('release-policy.conf') < s.index('release-policy.sig') < s.index('generate-integrity-manifest.py')
assert 'openssl dgst -sha256 -sign "$SIGNING_KEY"' in s
print('release policy detached-root-signature + manifest binding ordering: PASS')
PY
for n in 'format=5' 'trust_epoch=$TRUST_EPOCH' 'signer_key_id=$RELEASE_KEY_ID' 'transition=$TRANSITION' 'target_release_policy_sha256=$TARGET_POLICY_SHA256' 'NOVA_UPDATE_SIGNING_KEY'; do grep -Fq "$n" "$UPD"; done
for n in \
 'static int load_release_policy(void)' 'NOVA_RELEASE_POLICY_SIGNATURE' \
 'verify_release_policy_bundle(NOVA_RELEASE_POLICY,NOVA_RELEASE_POLICY_SIGNATURE' \
 'g_release_policy_verified=1' 'static int release_epoch_bounds(unsigned long *committed_epoch, unsigned long *pending_epoch)' \
 'checked_add_ul(g_tpm_trust_epoch_counter,1UL,&committed)' 'checked_add_ul(committed,1UL,&pending)' \
 'parse_release_policy_file' 'release_id_revoked(desc->signer_key_id)' \
 'g_release_next_key' 'unknown release signer rejected' 'revoked release signer rejected' \
 'target release policy sidecar rejected' 'same-epoch update attempted to mutate release trust policy'; do grep -Fq "$n" "$SRC"; done
! grep -Fq 'g_release_trust_epoch!=NOVA_TRUST_EPOCH' "$SRC"
! grep -Fq 'g_release_active_key_id,NOVA_RELEASE_KEY_ID' "$SRC"
echo 'Stage-4D root-signed delegated release-policy source contract passed.'
