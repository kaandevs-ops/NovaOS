#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"; D="$ROOT/scripts/build-disk.sh"; I="$ROOT/scripts/build-initramfs.sh"; P="$ROOT/scripts/verity/prepare-recovery-artifact.sh"; V="$ROOT/scripts/verity/validate-recovery-descriptor.py"; R="$ROOT/scripts/verity/build-recovery-slot.sh"; G="$ROOT/scripts/verity/build-verity-volume.sh"
for needle in \
  'NOVA_RECOVERY_DM_NODE "/dev/mapper/nova-recovery"' \
  'NOVA_RECOVERY_VERITY_MANIFEST_EMBEDDED "/Trust/recovery-verity.manifest"' \
  'NOVA_RECOVERY_VERITY_MANIFEST "/Data/Nova/Recovery/verity.manifest"' \
  'artifact_kind,"recovery-slot"' \
  'verify_recovery_verity_descriptor' \
  'setup_recovery_verity_mapping' \
  'nova_mount(NOVA_RECOVERY_DM_NODE,"/Recovery"' \
  'g_recovery_verity==VERITY_ENFORCED' 'firmware-initramfs' 'data-legacy'; do grep -Fq "$needle" "$SRC"; done
for needle in 'build-recovery-slot.sh' 'recovery.slot' 'recovery-slot' 'base-stage4d-recovery' 'validate-recovery-descriptor.py' 'openssl dgst -sha256 -sign'; do grep -Fq "$needle" "$P"; done
[[ -x "$V" ]]
grep -Fq 'Strict build-time validator for NovaOS Stage-4D embedded Recovery descriptors' "$V"
for needle in 'prepare-recovery-artifact.sh' 'recovery-verity.manifest' 'recovery-verity.sig'; do grep -Fq "$needle" "$I" "$D"; done
grep -Fq '20 24 Recovery' "$R"
for needle in 'veritysetup format' 'veritysetup verify' '--hash-offset=' 'payload_sha256=' 'verity_root_hash='; do grep -Fq -- "$needle" "$G"; done
echo "Stage-4C signed Recovery dm-verity source/build policy test passed."
