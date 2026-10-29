#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/init/main.c"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
init=s[s.index('static void initialize_tpm_policy'):s.index('static int read_efivar_u8')]
assert 'nova_tpm2_prepare_rollback_counter' in init
assert 'nova_tpm2_read_pcr_sha256' not in init, 'PCR health must not be conflated with NV counter initialization'
meas=s[s.index('static int perform_measured_boot'):s.index('static void copy_service')]
assert 'g_measurement_failed=1' in meas
assert 'g_tpm_state=TPM_STATE_FAILED' not in meas, 'PCR read/extend failures must not corrupt independent NV-counter health state'
for code in ('pcr7_read_failed','pcr14_read_failed','pcr14_extend_failed','pcr14_verify_read_failed','pcr14_extend_mismatch'):
    assert code in meas
state=s[s.index('static void write_boot_state'):s.index('static void cmd_boot')]
for key in ('measurement_failed=','pcr7_source=','pcr14_before_source=','pcr14_after_source=','tpm_last_rc=','tpm_device_node='):
    assert key in state
print('Stage-4C NV anti-rollback health and measured-boot health are independently tracked.')
PY
