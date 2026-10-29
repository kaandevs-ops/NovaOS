#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; S="$ROOT/src/init/main.c"
for needle in \
 'SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c' \
 'SetupMode-8be4df61-93ca-11d2-aa0d-00e098032b8c' \
 'nova_mount("efivarfs",NOVA_EFIVARS_DIR,"efivarfs"' \
 'g_firmware_trust=FIRMWARE_TRUST_VERIFIED' \
 'services blocked because the unified normal-boot authorization policy is not satisfied' \
 'firmware_trust=' \
 'firmware_secure_boot=' \
 'initramfs_binding=builtin-in-signed-efi-kernel' \
 'detect_firmware_trust();' \
 'mutation_health_ready()' \
 'normal_boot_policy_satisfied()'; do grep -Fq "$needle" "$S"; done
python3 - "$S" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
main=s[s.index('int nova_main(void)'):]
assert main.index('detect_firmware_trust();') < main.index('mount_stage4_storage();')
assert main.index('if(normal_boot_policy_satisfied())') < main.index('g_boot_stage="READY"')

policy=s[s.index('static int normal_boot_policy_satisfied(void) {'):s.index('static int boot_evidence_verified(void)')]
assert 'g_firmware_trust==FIRMWARE_TRUST_VERIFIED && g_firmware_secure_boot' in policy
assert 'g_tpm_state==TPM_STATE_READY' in policy
assert 'g_measured_boot && g_event_log_verified' in policy

services=s[s.index('static void start_enabled_services(void)'):s.index('static void stop_service')]
assert services.index('if(!normal_boot_policy_satisfied())') < services.index('start_service(&g_services[i])')

slot=s[s.index('static void slot_action'):s.index('static void persist_action')]
assert slot.index('mutation_health_ready()') < slot.index('commit_slot_metadata(target')
update=s[s.index('static void update_action'):s.index('static int recover_tpm_counter_commit_if_needed')]
apply=update[update.index('if(nova_streq(args,"apply"))'):]
assert apply.index('mutation_health_ready()') < apply.index('verify_update_bundle(&desc,1)')
print('Stage-4D UEFI trust + unified normal-boot/mutation authorization ordering passed.')
PY
echo "Stage-4D firmware trust/fail-closed ordering passed."
