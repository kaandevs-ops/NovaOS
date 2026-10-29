#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"
PREP="$ROOT/scripts/verity/prepare-recovery-artifact.sh"
VAL="$ROOT/scripts/verity/validate-recovery-descriptor.py"

for needle in \
  'parse_recovery_descriptor_v3_strict' \
  'parse_unsigned_canonical(value,&v) || v!=3UL' \
  '!nova_streq(value,"recovery-slot")' \
  '!nova_streq(value,"recovery.slot")' \
  'v!=NOVA_RECOVERY_SLOT_BYTES' \
  'v!=NOVA_RECOVERY_VERITY_DATA_BLOCKS' \
  'v!=NOVA_RECOVERY_VERITY_HASH_START_BLOCK' \
  'if(seen&bit) return 0' \
  'else return 0' \
  'seen==((1UL<<15)-1UL)' \
  'nova_streq(g_recovery_descriptor_source,"firmware-initramfs")' \
  'parse_verity_descriptor_file(manifest,out)'; do
  grep -Fq "$needle" "$SRC"
done

for needle in \
  'Strict build-time validator for NovaOS Stage-4D embedded Recovery descriptors' \
  'duplicate field' \
  'unknown field' \
  'leading zero is not canonical' \
  'unsigned 64-bit overflow' \
  'artifact_kind must be recovery-slot' \
  'Recovery rollback_index must be 0' \
  'payload must be recovery.slot' \
  'verity_hash_algorithm must be sha256'; do
  grep -Fq "$needle" "$VAL"
done

grep -Fq 'validate-recovery-descriptor.py" "$ART/verity.manifest"' "$PREP"
echo "Stage-4D strict firmware-bound Recovery descriptor source contract test passed."
