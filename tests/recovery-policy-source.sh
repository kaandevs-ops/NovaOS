#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
for needle in \
  'static int recovery_payload_verified(void)' \
  'TRUSTED DIAGNOSTIC (normal services and persistent mutations blocked)' \
  'INITRAMFS DIAGNOSTIC ONLY (Recovery payload not authorized)' \
  'Recovery trust does not override failed System, TPM, release-policy, or measured-boot authorization.' \
  'if(nova_streq(cmd,"recovery")){cmd_recovery();return 1;}' \
  'persist: mutation refused outside fully authorized normal boot' \
  'Data trust           : MUTABLE / UNTRUSTED DIAGNOSTIC INPUT' \
  'Descriptor source    :' 'firmware-initramfs' \
  'recovery_payload_trust=' 'persistent_mutation_authorized=' \
  'verified Recovery independently sealed read-only for normal or diagnostic use'; do
  grep -Fq "$needle" "$SRC"
done
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
r=s[s.index('static int recovery_payload_verified'):s.index('static void cmd_verity')]
for token in ('g_firmware_trust==FIRMWARE_TRUST_VERIFIED','g_firmware_secure_boot','g_recovery_verity_descriptor_verified','g_recovery_verity==VERITY_ENFORCED','g_recovery_verity_mapping_active','g_recovery_readonly'):
    assert token in r, token
assert 'mutation_health_ready()?"AUTHORIZED":"BLOCKED"' in r
p=s[s.index('static void persist_action'):s.index('static void banner')]
assert p.count('mutation_health_ready()') >= 2
main=s[s.index('int nova_main(void)'):]
assert main.index('g_recovery_readonly=seal_readonly_tree("/Recovery")') < main.index('if(g_storage_ready)')
mount=s[s.index('static void mount_stage4_storage'):s.index('static int release_id_revoked')]
data_fail=mount[mount.index('data_unavailable'):mount.index('initialize_tpm_policy()')]
assert 'return;' not in data_fail
assert mount.index('data_unavailable') < mount.index('verify_recovery_verity_descriptor(&rdesc)')
print('Stage-4D recovery shell semantics are diagnostic-only outside authorized normal boot.')
PY
