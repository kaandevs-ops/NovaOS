#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${1:-arm64}"
OUT="$ROOT/build/$ARCH"
ROOTFS="$OUT/rootfs"

PHASE16_ACCEPTANCE_PROBES="${NOVA_PHASE16_ACCEPTANCE_PROBES:-0}"
[[ "$PHASE16_ACCEPTANCE_PROBES" == 0 || "$PHASE16_ACCEPTANCE_PROBES" == 1 ]] || {
  echo "Invalid NOVA_PHASE16_ACCEPTANCE_PROBES (expected 0 or 1)" >&2
  exit 2
}
PHASE18_ACCEPTANCE_PROBES="${NOVA_PHASE18_ACCEPTANCE_PROBES:-0}"
[[ "$PHASE18_ACCEPTANCE_PROBES" == 0 || "$PHASE18_ACCEPTANCE_PROBES" == 1 ]] || { echo "Invalid NOVA_PHASE18_ACCEPTANCE_PROBES" >&2; exit 2; }
STEP7_PROBES="${NOVA_PHASE18_STEP7_PROBES:-0}"
[[ "$STEP7_PROBES" == 0 || "$STEP7_PROBES" == 1 ]] || { echo 'Invalid NOVA_PHASE18_STEP7_PROBES' >&2; exit 2; }
STEP6_PROBES="${NOVA_PHASE18_STEP6_PROBES:-0}"
[[ "$STEP6_PROBES" == 0 || "$STEP6_PROBES" == 1 ]] || { echo 'Invalid NOVA_PHASE18_STEP6_PROBES' >&2; exit 2; }
STEP5_PROBES="${NOVA_PHASE18_STEP5_PROBES:-0}"
[[ "$STEP5_PROBES" == 0 || "$STEP5_PROBES" == 1 ]] || { echo "Invalid NOVA_PHASE18_STEP5_PROBES" >&2; exit 2; }
PHASE17_ACCEPTANCE_PROBES="${NOVA_PHASE17_ACCEPTANCE_PROBES:-0}"
[[ "$PHASE17_ACCEPTANCE_PROBES" == 0 || "$PHASE17_ACCEPTANCE_PROBES" == 1 ]] || {
  echo "Invalid NOVA_PHASE17_ACCEPTANCE_PROBES (expected 0 or 1)" >&2
  exit 2
}
if [[ "$STEP5_PROBES" == 1 && ( "$PHASE18_ACCEPTANCE_PROBES" != 1 ||
      "${NOVA_PHASE18_STEP3_ADVERSARIAL:-0}" != 0 || "${NOVA_PHASE18_STEP4_PROBES:-0}" != 0 ) ]]; then
  echo 'Step-5 probes require Phase-18 acceptance mode exclusively' >&2
  exit 2
fi
if [[ "$STEP7_PROBES" == 1 && ( "$PHASE18_ACCEPTANCE_PROBES" != 1 ||
      "$STEP6_PROBES" != 0 || "${NOVA_PHASE18_STEP5_PROBES:-0}" != 0 ||
      "${NOVA_PHASE18_STEP4_PROBES:-0}" != 0 ||
      "${NOVA_PHASE18_STEP3_ADVERSARIAL:-0}" != 0 ||
      "${NOVA_PHASE16_ACCEPTANCE_PROBES:-0}" != 0 ||
      "${NOVA_PHASE17_ACCEPTANCE_PROBES:-0}" != 0 ) ]]; then
  echo 'Step-7 probes require Phase-18 acceptance-only mode' >&2
  exit 2
fi
if [[ "$STEP6_PROBES" == 1 && ( "$PHASE18_ACCEPTANCE_PROBES" != 1 ||
      "$STEP5_PROBES" != 0 || "${NOVA_PHASE18_STEP4_PROBES:-0}" != 0 ||
      "${NOVA_PHASE18_STEP3_ADVERSARIAL:-0}" != 0 ) ]]; then
  echo 'Step-6 probes require Phase-18 acceptance mode exclusively' >&2
  exit 2
fi
if (( PHASE16_ACCEPTANCE_PROBES + PHASE17_ACCEPTANCE_PROBES + PHASE18_ACCEPTANCE_PROBES > 1 )); then
  echo "Phase-16 and Phase-17 acceptance probes are mutually exclusive" >&2
  exit 2
fi

# Phase-18 Step-3 R3: fail closed if an acceptance-only rootfs is reused as
# a normal image. NEVER silently carry probe executables/service manifests
# into an image with acceptance flags disabled. Use a fresh build tree.
if [[ "$PHASE16_ACCEPTANCE_PROBES" == 0 &&
      "$PHASE17_ACCEPTANCE_PROBES" == 0 &&
      "$PHASE18_ACCEPTANCE_PROBES" == 0 ]]; then
  for probe_path in \
    "$ROOTFS"/System/Apps/nova-security-probe-* \
    "$ROOTFS"/System/Apps/nova-sandbox-probe-* \
    "$ROOTFS"/System/Apps/nova-resource-probe-* \
    "$ROOTFS"/System/Services/phase16-*.service \
    "$ROOTFS"/System/Services/phase17-*.service \
    "$ROOTFS"/System/Services/phase18-*.service; do
    if [[ -e "$probe_path" || -L "$probe_path" ]]; then
      printf 'FAIL-CLOSED: acceptance-only artifact in normal build: %s\n' "$probe_path" >&2
      echo 'Use a fresh isolated build tree; no existing artifacts were removed.' >&2
      exit 2
    fi
  done
  if [[ -f "$ROOTFS/System/Core/nova-resource-broker" ]] &&
     LC_ALL=C grep -aFq 'step3_fixture_mode=adversarial-only' "$ROOTFS/System/Core/nova-resource-broker"; then
    echo 'FAIL-CLOSED: acceptance-only resource-broker binary in normal build.' >&2
    exit 2
  fi
fi

NEED_USERLAND=0
if [[ ! -x "$ROOTFS/System/Core/nova-terminal-session" || ! -x "$ROOTFS/System/Terminal/bin/busybox" || ! -x "$ROOTFS/System/Apps/nova-files" || ! -x "$ROOTFS/init" || ! -x "$ROOTFS/System/Core/nova-heartbeat" || ! -x "$ROOTFS/System/Core/nova-compositor" || ! -x "$ROOTFS/System/Core/nova-assets" || ! -x "$ROOTFS/System/Core/nova-resource-broker" || ! -x "$ROOTFS/System/Core/nova-shell" || ! -x "$ROOTFS/System/Apps/nova-demo-app" || ! -x "$ROOTFS/System/Apps/nova-demo-app-b" ]]; then
  NEED_USERLAND=1
fi
if [[ "$PHASE16_ACCEPTANCE_PROBES" == 1 ]]; then
  for probe in nova-security-probe-read nova-security-probe-no-ui nova-security-probe-write nova-security-probe-unregistered; do
    [[ -x "$ROOTFS/System/Apps/$probe" ]] || NEED_USERLAND=1
  done
fi
if [[ "$PHASE17_ACCEPTANCE_PROBES" == 1 ]]; then
  for probe in nova-sandbox-probe-a nova-sandbox-probe-b; do
    [[ -x "$ROOTFS/System/Apps/$probe" ]] || NEED_USERLAND=1
  done
fi
if [[ "$STEP7_PROBES" == 1 ]]; then
  [[ -x "$ROOTFS/System/Apps/nova-step7-idle-stress" ]] || NEED_USERLAND=1
elif [[ "$STEP6_PROBES" == 1 ]]; then
  [[ -x "$ROOTFS/System/Apps/nova-step6-session-stress" ]] || NEED_USERLAND=1
elif [[ "$STEP5_PROBES" == 1 ]]; then
  for role in demo notes both none; do
    [[ -x "$ROOTFS/System/Apps/nova-resource-probe-$role" ]] || NEED_USERLAND=1
  done
elif [[ "$PHASE18_ACCEPTANCE_PROBES" == 1 ]]; then
  for probe in nova-resource-probe-grant nova-resource-probe-deny; do
    [[ -x "$ROOTFS/System/Apps/$probe" ]] || NEED_USERLAND=1
  done
fi
if [[ "$NEED_USERLAND" == 1 ]]; then
  "$ROOT/scripts/build-userland.sh" "$ARCH"
fi

mkdir -p "$ROOTFS"/{dev,proc,sys,run,tmp,Applications,System/Services,System/Integrity,System/Trust,System/Resources,Trust,Users,Data,Recovery}
# Only broker knows this verified image-owned resource. It is absent from the
# Phase-17 application's private /System namespace; access requires SCM_RIGHTS.
install -m 0444 "$ROOT/configs/resources/phase18-broker-demo.txt" \
  "$ROOTFS/System/Resources/phase18-broker-demo.txt"
install -m 0444 "$ROOT/configs/resources/phase18-broker-notes.txt" \
  "$ROOTFS/System/Resources/phase18-broker-notes.txt"
chmod 1777 "$ROOTFS/tmp"
printf 'NovaOS Stage-5A Phase-5\n' > "$ROOTFS/System/release"
# Phase-16/17 legacy acceptance probes deliberately own client 3 in their isolated fixture.
# Keep the normal production registry intact; omit Files only from those probe registries.
while IFS= read -r entry; do
  if [[ ( "$PHASE16_ACCEPTANCE_PROBES" == 1 || "$PHASE17_ACCEPTANCE_PROBES" == 1 ) && "$entry" == /System/Services/nova-files.service ]]; then continue; fi
  printf '%s\n' "$entry"
done < "$ROOT/configs/services/services.list" > "$ROOTFS/System/Services/services.list"
cp "$ROOT/configs/services/heartbeat.service" "$ROOTFS/System/Services/heartbeat.service"
cp "$ROOT/configs/services/compositor.service" "$ROOTFS/System/Services/compositor.service"
cp "$ROOT/configs/services/assets.service" "$ROOTFS/System/Services/assets.service"
cp "$ROOT/configs/services/resource-broker.service" "$ROOTFS/System/Services/resource-broker.service"
cp "$ROOT/configs/services/shell.service" "$ROOTFS/System/Services/shell.service"
cp "$ROOT/configs/services/demo-app.service" "$ROOTFS/System/Services/demo-app.service"
cp "$ROOT/configs/services/demo-app-b.service" "$ROOTFS/System/Services/demo-app-b.service"
cp "$ROOT/configs/services/nova-files.service" "$ROOTFS/System/Services/nova-files.service"
if [[ "${NOVA_R47_BROWSER_COMPAT:-0}" == 1 ]]; then
  [[ "$ARCH" == x86_64 && -x "$ROOTFS/System/Apps/nova-browser-session" && -x "$ROOTFS/System/Browser/opt/firefox/firefox" ]] && LC_ALL=C grep -aqF "browser-compat-v1" "$ROOTFS/init" || { echo "Browser runtime staging incomplete" >&2; exit 2; }
  install -d -m 0755 "$ROOTFS/System/Browser/usr/share/nova"
  install -m 0444 "$ROOT/assets/browser/start.html" "$ROOTFS/System/Browser/usr/share/nova/start.html"
  # R59: offline web applications stay in the existing read-only browser runtime.
  python3 "$ROOT/tools/r59-stage-webapps.py" "$ROOTFS/System/Browser/usr/share/nova/apps"
  cp "$ROOT/configs/services/nova-browser.service.pending" "$ROOTFS/System/Services/nova-browser.service"
  printf '%s\n' /System/Services/nova-browser.service >> "$ROOTFS/System/Services/services.list"
fi

if [[ "$PHASE16_ACCEPTANCE_PROBES" == 1 ]]; then
  FIX="$ROOT/tests/fixtures/stage5a-phase16"
  cp "$FIX/security-probe-read.service" "$ROOTFS/System/Services/phase16-security-probe-read.service"
  cp "$FIX/security-probe-no-ui.service" "$ROOTFS/System/Services/phase16-security-probe-no-ui.service"
  cp "$FIX/security-probe-write.service" "$ROOTFS/System/Services/phase16-security-probe-write.service"
  cp "$FIX/security-probe-unregistered.service" "$ROOTFS/System/Services/phase16-security-probe-unregistered.service"
  cat >> "$ROOTFS/System/Services/services.list" <<'PHASE16PROBES'
/System/Services/phase16-security-probe-read.service
/System/Services/phase16-security-probe-no-ui.service
/System/Services/phase16-security-probe-write.service
/System/Services/phase16-security-probe-unregistered.service
PHASE16PROBES
fi
if [[ "$PHASE17_ACCEPTANCE_PROBES" == 1 ]]; then
  FIX="$ROOT/tests/fixtures/stage5a-phase17"
  cp "$FIX/sandbox-probe-a.service" "$ROOTFS/System/Services/phase17-sandbox-probe-a.service"
  cp "$FIX/sandbox-probe-b.service" "$ROOTFS/System/Services/phase17-sandbox-probe-b.service"
  cat >> "$ROOTFS/System/Services/services.list" <<'PHASE17PROBES'
/System/Services/phase17-sandbox-probe-a.service
/System/Services/phase17-sandbox-probe-b.service
PHASE17PROBES
fi
if [[ "$STEP7_PROBES" == 1 ]]; then
  cp "$ROOT/tests/fixtures/stage5a-phase18-step7/idle-stress.service" \
    "$ROOTFS/System/Services/phase18-step7-idle-stress.service"
  echo "/System/Services/phase18-step7-idle-stress.service" >> "$ROOTFS/System/Services/services.list"
elif [[ "$STEP6_PROBES" == 1 ]]; then
  cp "$ROOT/tests/fixtures/stage5a-phase18-step6/session-stress.service" \
    "$ROOTFS/System/Services/phase18-step6-session-stress.service"
  echo "/System/Services/phase18-step6-session-stress.service" >> "$ROOTFS/System/Services/services.list"
elif [[ "$STEP5_PROBES" == 1 ]]; then
  FIX="$ROOT/tests/fixtures/stage5a-phase18-step5"
  for role in demo notes both none; do
    cp "$FIX/resource-probe-$role.service" "$ROOTFS/System/Services/phase18-resource-probe-$role.service"
    echo "/System/Services/phase18-resource-probe-$role.service" >> "$ROOTFS/System/Services/services.list"
  done
elif [[ "$PHASE18_ACCEPTANCE_PROBES" == 1 ]]; then
  FIX="$ROOT/tests/fixtures/stage5a-phase18"
  for probe in grant deny; do
    cp "$FIX/resource-probe-$probe.service" "$ROOTFS/System/Services/phase18-resource-probe-$probe.service"
    echo "/System/Services/phase18-resource-probe-$probe.service" >> "$ROOTFS/System/Services/services.list"
  done
fi
cp "$ROOT/configs/recovery/recovery.conf" "$ROOTFS/Recovery/recovery.conf"
cp "$ROOT/configs/recovery/README.txt" "$ROOTFS/Recovery/README.txt"

ROOT_CERT_DER="$("$ROOT/scripts/prepare-trust.sh" "$ARCH")"
cp "$ROOT_CERT_DER" "$ROOTFS/System/Trust/nova-root.der"
cp "$ROOT_CERT_DER" "$ROOTFS/Trust/nova-root.der"
CERT_SHA256="$(sha256sum "$ROOTFS/System/Trust/nova-root.der" | awk '{print $1}')"
TRUST_PROFILE="${NOVA_TRUST_PROFILE:-development}"
TRUST_KEY_ID="${NOVA_TRUST_KEY_ID:-nova-development-root-2026}"
cat > "$ROOTFS/System/Trust/trust.conf" <<TRUSTCONF
profile=$TRUST_PROFILE
key_id=$TRUST_KEY_ID
algorithm=rsa-pkcs1-sha256
cert_sha256=$CERT_SHA256
manifest=/System/Integrity/manifest.sha256
signature=/System/Integrity/manifest.sig
TRUSTCONF

UEFI_CERT_PEM="$($ROOT/scripts/uefi/prepare-uefi-trust.sh "$ARCH")"
UEFI_CERT_DER="$ROOT/build/$ARCH/uefi-trust/uefi-db.crt.der"
UEFI_CERT_SHA256="$(sha256sum "$UEFI_CERT_DER" | awk '{print $1}')"
UEFI_PK_SHA256="$(sha256sum "$ROOT/build/$ARCH/uefi-trust/uefi-pk.crt.der" | awk '{print $1}')"
UEFI_KEK_SHA256="$(sha256sum "$ROOT/build/$ARCH/uefi-trust/uefi-kek.crt.der" | awk '{print $1}')"
cat > "$ROOTFS/Trust/uefi-boot.conf" <<UEFICONF
profile=${NOVA_UEFI_TRUST_PROFILE:-development}
mode=uefi-secure-boot
pk_cert_sha256=$UEFI_PK_SHA256
kek_cert_sha256=$UEFI_KEK_SHA256
db_signer_cert_sha256=$UEFI_CERT_SHA256
signer_cert_sha256=$UEFI_CERT_SHA256
dbx_policy=foundation-empty
dbx_firmware_enrollment=not-performed-by-stage4d
initramfs_binding=builtin-in-signed-efi-kernel
UEFICONF
cp "$ROOTFS/Trust/uefi-boot.conf" "$ROOTFS/System/Trust/uefi-boot.conf"

# Stage-4D delegated release-signing policy.  The policy is independently
# signed by the offline/root content key and is also covered by the signed
# System integrity manifest + dm-verity.  Development builds generate local
# keys under build/ only; private keys are never copied into rootfs/packages.
if [[ -n "${NOVA_ROOT_CERT:-}" && -z "${NOVA_SIGNING_KEY:-}" ]]; then
  echo "ERROR: NOVA_ROOT_CERT was supplied without matching NOVA_SIGNING_KEY." >&2
  exit 2
fi
if [[ -n "${NOVA_SIGNING_KEY:-}" ]]; then
  SIGNING_KEY="$NOVA_SIGNING_KEY"
else
  "$ROOT/scripts/ensure-dev-key.sh" >/dev/null
  SIGNING_KEY="$ROOT/build/dev-keys/NOVA-DEVELOPMENT-ONLY.key.pem"
fi
[[ -f "$SIGNING_KEY" ]] || { echo "ERROR: NovaOS root signing key not found: $SIGNING_KEY" >&2; exit 2; }

# Recovery trust must remain available even when mutable /Data cannot mount.
# Prepare the fixed Recovery image/descriptor before initramfs packaging and
# bind the root-signed descriptor into /Trust, which is itself authenticated by
# the Secure-Boot-signed EFI-stub kernel + embedded initramfs.
RECOVERY_ARTIFACT="$OUT/recovery-artifact"
# Regenerate on every initramfs build so Recovery source/config changes cannot
# silently reuse a stale verity root or descriptor from an earlier build.
"$ROOT/scripts/verity/prepare-recovery-artifact.sh" "$ARCH"
cp "$RECOVERY_ARTIFACT/verity.manifest" "$ROOTFS/Trust/recovery-verity.manifest"
cp "$RECOVERY_ARTIFACT/verity.sig" "$ROOTFS/Trust/recovery-verity.sig"

RELEASE_KEY_ID="${NOVA_RELEASE_KEY_ID:-nova-development-release-2026-a}"
TRUST_EPOCH="${NOVA_TRUST_EPOCH:-1}"
RELEASE_NEXT_KEY_ID="${NOVA_RELEASE_NEXT_KEY_ID:-none}"
RELEASE_REVOKED_KEY_IDS="${NOVA_RELEASE_REVOKED_KEY_IDS:-none}"
[[ "$RELEASE_KEY_ID" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid NOVA_RELEASE_KEY_ID" >&2; exit 2; }
[[ "$RELEASE_NEXT_KEY_ID" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid NOVA_RELEASE_NEXT_KEY_ID" >&2; exit 2; }
[[ "$RELEASE_REVOKED_KEY_IDS" =~ ^[A-Za-z0-9._,-]+$ ]] || { echo "Invalid NOVA_RELEASE_REVOKED_KEY_IDS" >&2; exit 2; }
[[ "$TRUST_EPOCH" =~ ^[0-9]+$ ]] || { echo "Invalid NOVA_TRUST_EPOCH" >&2; exit 2; }
validate_revocation_ids() {
  local raw="$1" id
  [[ "$raw" == none ]] && return 0
  local -A seen=()
  IFS=',' read -r -a ids <<< "$raw"
  ((${#ids[@]} > 0)) || return 1
  for id in "${ids[@]}"; do
    [[ "$id" =~ ^[A-Za-z0-9._-]+$ && "$id" != none ]] || return 1
    [[ -z "${seen[$id]+x}" ]] || return 1
    seen[$id]=1
  done
}
validate_revocation_ids "$RELEASE_REVOKED_KEY_IDS" || { echo "Invalid/duplicate NOVA_RELEASE_REVOKED_KEY_IDS" >&2; exit 2; }
[[ ",$RELEASE_REVOKED_KEY_IDS," != *",$RELEASE_KEY_ID,"* ]] || { echo "Active release key may not be revoked" >&2; exit 2; }
[[ "$RELEASE_NEXT_KEY_ID" == none || "$RELEASE_NEXT_KEY_ID" != "$RELEASE_KEY_ID" ]] || { echo "Next release key must differ from active key" >&2; exit 2; }
[[ "$RELEASE_NEXT_KEY_ID" == none || ",$RELEASE_REVOKED_KEY_IDS," != *",$RELEASE_NEXT_KEY_ID,"* ]] || { echo "Next release key may not already be revoked" >&2; exit 2; }

if [[ -n "${NOVA_RELEASE_CERT:-}" ]]; then
  RELEASE_CERT_SRC="$NOVA_RELEASE_CERT"
else
  RELEASE_CERT_SRC="$("$ROOT/scripts/ensure-dev-release-key.sh")"
fi
RELEASE_CERT_DER="$ROOTFS/System/Trust/release-active.der"
if openssl x509 -inform DER -in "$RELEASE_CERT_SRC" -noout >/dev/null 2>&1; then
  cp "$RELEASE_CERT_SRC" "$RELEASE_CERT_DER"
elif openssl x509 -in "$RELEASE_CERT_SRC" -noout >/dev/null 2>&1; then
  openssl x509 -in "$RELEASE_CERT_SRC" -outform DER -out "$RELEASE_CERT_DER"
else
  echo "ERROR: NOVA_RELEASE_CERT is not a readable X.509 PEM/DER certificate." >&2
  exit 2
fi
RELEASE_CERT_SHA256="$(sha256sum "$RELEASE_CERT_DER" | awk '{print $1}')"

RELEASE_NEXT_CERT_PATH=none
RELEASE_NEXT_CERT_SHA256=none
python3 "$ROOT/scripts/retain-build-files.py" "$ROOTFS/System/Trust/release-next.der"
if [[ "$RELEASE_NEXT_KEY_ID" != none ]]; then
  if [[ -n "${NOVA_RELEASE_NEXT_CERT:-}" ]]; then
    RELEASE_NEXT_CERT_SRC="$NOVA_RELEASE_NEXT_CERT"
  else
    RELEASE_NEXT_CERT_SRC="$("$ROOT/scripts/ensure-dev-release-next-key.sh")"
  fi
  RELEASE_NEXT_CERT_DER="$ROOTFS/System/Trust/release-next.der"
  if openssl x509 -inform DER -in "$RELEASE_NEXT_CERT_SRC" -noout >/dev/null 2>&1; then
    cp "$RELEASE_NEXT_CERT_SRC" "$RELEASE_NEXT_CERT_DER"
  elif openssl x509 -in "$RELEASE_NEXT_CERT_SRC" -noout >/dev/null 2>&1; then
    openssl x509 -in "$RELEASE_NEXT_CERT_SRC" -outform DER -out "$RELEASE_NEXT_CERT_DER"
  else
    echo "ERROR: NOVA_RELEASE_NEXT_CERT is not a readable X.509 PEM/DER certificate." >&2
    exit 2
  fi
  RELEASE_NEXT_CERT_PATH=/System/Trust/release-next.der
  RELEASE_NEXT_CERT_SHA256="$(sha256sum "$RELEASE_NEXT_CERT_DER" | awk '{print $1}')"
fi

# Trust-domain separation is enforced on the certificate public key (SPKI),
# not merely on certificate bytes, filenames, or logical key IDs.  This also
# catches the unsafe case where one private key is wrapped in multiple distinct
# X.509 certificates.  PK/KEK/db may deliberately share one UEFI development
# key, but UEFI firmware trust, the NovaOS content root, and delegated release
# signers must remain cryptographically distinct.
cert_spki_sha256() {
  local cert_der="$1"
  openssl x509 -inform DER -in "$cert_der" -pubkey -noout 2>/dev/null |
    openssl pkey -pubin -outform DER 2>/dev/null |
    sha256sum | awk '{print $1}'
}
CONTENT_ROOT_SPKI_SHA256="$(cert_spki_sha256 "$ROOTFS/System/Trust/nova-root.der")"
RELEASE_SPKI_SHA256="$(cert_spki_sha256 "$RELEASE_CERT_DER")"
UEFI_PK_SPKI_SHA256="$(cert_spki_sha256 "$ROOT/build/$ARCH/uefi-trust/uefi-pk.crt.der")"
UEFI_KEK_SPKI_SHA256="$(cert_spki_sha256 "$ROOT/build/$ARCH/uefi-trust/uefi-kek.crt.der")"
UEFI_DB_SPKI_SHA256="$(cert_spki_sha256 "$ROOT/build/$ARCH/uefi-trust/uefi-db.crt.der")"
for uefi_spki in "$UEFI_PK_SPKI_SHA256" "$UEFI_KEK_SPKI_SHA256" "$UEFI_DB_SPKI_SHA256"; do
  [[ "$CONTENT_ROOT_SPKI_SHA256" != "$uefi_spki" ]] || { echo "ERROR: NovaOS content-root public key must not reuse a UEFI PK/KEK/db public key." >&2; exit 2; }
  [[ "$RELEASE_SPKI_SHA256" != "$uefi_spki" ]] || { echo "ERROR: delegated release public key must not reuse a UEFI PK/KEK/db public key." >&2; exit 2; }
done
[[ "$RELEASE_SPKI_SHA256" != "$CONTENT_ROOT_SPKI_SHA256" ]] || { echo "ERROR: delegated release public key must not reuse the NovaOS content-root public key." >&2; exit 2; }
if [[ "$RELEASE_NEXT_KEY_ID" != none ]]; then
  RELEASE_NEXT_SPKI_SHA256="$(cert_spki_sha256 "$RELEASE_NEXT_CERT_DER")"
  [[ "$RELEASE_NEXT_SPKI_SHA256" != "$RELEASE_SPKI_SHA256" ]] || { echo "ERROR: active and next delegated release public keys must be distinct." >&2; exit 2; }
  [[ "$RELEASE_NEXT_SPKI_SHA256" != "$CONTENT_ROOT_SPKI_SHA256" ]] || { echo "ERROR: next delegated release public key must not reuse the NovaOS content-root public key." >&2; exit 2; }
  for uefi_spki in "$UEFI_PK_SPKI_SHA256" "$UEFI_KEK_SPKI_SHA256" "$UEFI_DB_SPKI_SHA256"; do
    [[ "$RELEASE_NEXT_SPKI_SHA256" != "$uefi_spki" ]] || { echo "ERROR: next delegated release public key must not reuse a UEFI PK/KEK/db public key." >&2; exit 2; }
  done
fi

POLICY_STATE=stable
[[ "$RELEASE_NEXT_KEY_ID" != none ]] && POLICY_STATE=rotation-prepared
cat > "$ROOTFS/System/Trust/release-policy.conf" <<RELEASEPOLICY
format=3
policy_id=novaos-release-policy
policy_state=$POLICY_STATE
signature_algorithm=rsa-pkcs1-sha256
trust_epoch=$TRUST_EPOCH
active_key_id=$RELEASE_KEY_ID
active_cert=release-active.der
active_cert_sha256=$RELEASE_CERT_SHA256
next_key_id=$RELEASE_NEXT_KEY_ID
next_cert=${RELEASE_NEXT_CERT_PATH##*/}
next_cert_sha256=$RELEASE_NEXT_CERT_SHA256
revoked_key_ids=$RELEASE_REVOKED_KEY_IDS
policy_binding=root-detached-signature+root-signed-system-manifest+dm-verity+TPM-trust-epoch
RELEASEPOLICY
python3 "$ROOT/scripts/validate-release-policy.py" \
  "$ROOTFS/System/Trust/release-policy.conf" \
  --expect-epoch "$TRUST_EPOCH" --expect-active "$RELEASE_KEY_ID" >/dev/null
openssl dgst -sha256 -sign "$SIGNING_KEY" \
  -out "$ROOTFS/System/Trust/release-policy.sig" \
  "$ROOTFS/System/Trust/release-policy.conf"


python3 "$ROOT/scripts/retain-build-files.py" "$ROOTFS/System/Integrity/manifest.sha256" "$ROOTFS/System/Integrity/manifest.sig"
"$ROOT/scripts/generate-integrity-manifest.py" "$ROOTFS"

openssl dgst -sha256 -sign "$SIGNING_KEY" \
  -out "$ROOTFS/System/Integrity/manifest.sig" \
  "$ROOTFS/System/Integrity/manifest.sha256"
# Catch mismatched external key/certificate at build time instead of shipping a dead boot chain.
openssl x509 -inform DER -in "$ROOTFS/System/Trust/nova-root.der" -pubkey -noout > "$OUT/root-public-key.pem"
openssl dgst -sha256 -verify "$OUT/root-public-key.pem" \
  -signature "$ROOTFS/System/Trust/release-policy.sig" \
  "$ROOTFS/System/Trust/release-policy.conf" >/dev/null
openssl dgst -sha256 -verify "$OUT/root-public-key.pem" \
  -signature "$ROOTFS/System/Integrity/manifest.sig" \
  "$ROOTFS/System/Integrity/manifest.sha256" >/dev/null

SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-0}"
python3 - "$ROOTFS" "$SOURCE_DATE_EPOCH" <<'PYMTIME'
import os, sys
root, epoch = sys.argv[1], int(sys.argv[2])
for base, dirs, files in os.walk(root):
    for name in dirs + files:
        path = os.path.join(base, name)
        try:
            os.utime(path, (epoch, epoch), follow_symlinks=False)
        except (NotImplementedError, OSError):
            pass
os.utime(root, (epoch, epoch))
PYMTIME

FILES_CPIO="$OUT/initramfs.files.cpio"
DEVICES_CPIO="$OUT/initramfs.devices.cpio"
(
  cd "$ROOTFS"
  if [[ "${NOVA_R47_BROWSER_COMPAT:-0}" == 1 ]]; then
    # Runtime is covered by signed System dm-verity, mounted before integrity
    # verification/services. Keep it out of the EFI embedded RAM filesystem.
    find . -path './System/Browser' -prune -o -print0
  else
    find . -print0
  fi | LC_ALL=C sort -z | cpio --null -o --format=newc --owner=0:0 --reproducible 2>/dev/null > "$FILES_CPIO"
)
# The kernel opens /dev/console before PID 1 can mount devtmpfs.  A plain file
# or an empty /dev directory is insufficient for an embedded initramfs.  Build
# the char-device entry without requiring root/CAP_MKNOD on the build host.
python3 "$ROOT/scripts/initramfs-device-cpio.py" "$DEVICES_CPIO" --epoch "$SOURCE_DATE_EPOCH"
cat "$FILES_CPIO" "$DEVICES_CPIO" > "$OUT/initramfs.cpio"
python3 "$ROOT/scripts/retain-build-files.py" "$FILES_CPIO" "$DEVICES_CPIO"
gzip -9n -c "$OUT/initramfs.cpio" > "$OUT/initramfs.cpio.gz"

echo "Built: $OUT/initramfs.cpio"
echo "Built: $OUT/initramfs.cpio.gz"
