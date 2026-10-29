#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"; INITRD="$ROOT/scripts/build-initramfs.sh"
for needle in \
  '#define PREBOOT_ROOT_CERT_PATH "/Trust/nova-root.der"' \
  'ensure_pinned_root_key(PREBOOT_ROOT_CERT_PATH)' \
  'verify_signature_with_loaded_root(manifest,signature)' \
  'verify_slot_verity_descriptor(g_active_slot,&vdesc)' \
  'verify_recovery_verity_descriptor(&rdesc)' \
  'setup_system_verity_mapping' \
  'setup_recovery_verity_mapping'; do grep -Fq "$needle" "$SRC"; done
grep -Fq 'cp "$ROOT_CERT_DER" "$ROOTFS/Trust/nova-root.der"' "$INITRD"
grep -Fq 'cp "$RECOVERY_ARTIFACT/verity.manifest" "$ROOTFS/Trust/recovery-verity.manifest"' "$INITRD"
grep -Fq 'NOVA_RECOVERY_VERITY_MANIFEST_EMBEDDED "/Trust/recovery-verity.manifest"' "$SRC"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text(); m=s[s.index('static void mount_stage4_storage'):s.index('struct nova_update_descriptor')]
assert m.index('verify_slot_verity_descriptor') < m.index('setup_system_verity_mapping') < m.index('nova_mount(NOVA_DM_NODE')
assert m.index('verify_recovery_verity_descriptor') < m.index('setup_recovery_verity_mapping') < m.index('nova_mount(NOVA_RECOVERY_DM_NODE')
print('Stage-4A pre-mount System + Recovery trust ordering verified.')
PY
echo "Stage-4A preboot verity trust test passed."
