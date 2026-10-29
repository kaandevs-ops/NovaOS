#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
V="$ROOT/scripts/validate-release-policy.py"
UPD="$ROOT/scripts/build-update-bundle.sh"
INIT="$ROOT/scripts/build-initramfs.sh"

for needle in \
  'duplicate field' 'unknown field' 'non-canonical leading zero' \
  'value outside supported unsigned-long range' \
  'promote-next target epoch must be previous epoch + 1' \
  'promote-next target policy must revoke the previous active key' \
  'promote-next target policy attempted to un-revoke' \
  'promote-next target active certificate is not the exact previous next certificate'; do
  grep -Fq "$needle" "$V"
done
for needle in 'validate-release-policy.py' '--expect-epoch "$TRUST_EPOCH"' '--expect-active "$RELEASE_KEY_ID"'; do
  grep -Fq -- "$needle" "$INIT" "$UPD"
done
for needle in '--previous "$PREVIOUS_POLICY"' '--promote-next'; do
  grep -Fq -- "$needle" "$UPD"
done

# Functional vectors run later as part of the Stage-4D source suite.  They are
# intentionally tiny and do not boot QEMU or touch TPM state.
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
policy() {
  local out="$1" epoch="$2" active="$3" next="$4" revoked="$5" state=stable next_cert=none next_hash=none
  if [[ "$next" != none ]]; then state=rotation-prepared; next_cert=release-next.der; next_hash=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb; fi
  cat > "$out" <<EOF
format=3
policy_id=novaos-release-policy
policy_state=$state
signature_algorithm=rsa-pkcs1-sha256
trust_epoch=$epoch
active_key_id=$active
active_cert=release-active.der
active_cert_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
next_key_id=$next
next_cert=$next_cert
next_cert_sha256=$next_hash
revoked_key_ids=$revoked
policy_binding=root-detached-signature+root-signed-system-manifest+dm-verity+TPM-trust-epoch
EOF
}
policy "$tmp/prev" 1 key-a key-b old-revoked
policy "$tmp/next" 2 key-b none key-a,old-revoked
sed -i 's/^active_cert_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa$/active_cert_sha256=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb/' "$tmp/next"
python3 "$V" "$tmp/prev" --expect-epoch 1 --expect-active key-a >/dev/null
python3 "$V" "$tmp/next" --expect-epoch 2 --expect-active key-b --previous "$tmp/prev" --promote-next >/dev/null
cp "$tmp/next" "$tmp/wrong-cert"
sed -i 's/^active_cert_sha256=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb$/active_cert_sha256=cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc/' "$tmp/wrong-cert"
if python3 "$V" "$tmp/wrong-cert" --previous "$tmp/prev" --promote-next >/dev/null 2>&1; then echo 'different certificate under promoted key id unexpectedly accepted' >&2; exit 1; fi
cp "$tmp/next" "$tmp/dup"; printf 'trust_epoch=2\n' >> "$tmp/dup"
if python3 "$V" "$tmp/dup" >/dev/null 2>&1; then echo 'duplicate release-policy field unexpectedly accepted' >&2; exit 1; fi
policy "$tmp/unrevoke" 2 key-b none key-a
sed -i 's/^active_cert_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa$/active_cert_sha256=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb/' "$tmp/unrevoke"
if python3 "$V" "$tmp/unrevoke" --previous "$tmp/prev" --promote-next >/dev/null 2>&1; then echo 'revocation rollback unexpectedly accepted' >&2; exit 1; fi
policy "$tmp/overflow" 18446744073709551616 key-a none none
if python3 "$V" "$tmp/overflow" >/dev/null 2>&1; then echo 'overflow trust epoch unexpectedly accepted' >&2; exit 1; fi

echo 'Stage-4D strict host release-policy validator source/functional contract passed.'
