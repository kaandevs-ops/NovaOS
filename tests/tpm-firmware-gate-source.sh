#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/init/main.c"
PYTHON=${PYTHON:-python3}
for needle in \
  'if(g_firmware_trust!=FIRMWARE_TRUST_VERIFIED)' \
  'TPM NV anti-rollback state is not provisioned or mutated without verified firmware trust' \
  'g_tpm_state=TPM_STATE_UNAVAILABLE'; do
  grep -Fq "$needle" "$SRC"
done
"$PYTHON" - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
f=s.index('static void initialize_tpm_policy(void)')
end=s.index('static int read_efivar_u8', f)
b=s[f:end]
present=b.index('nova_tpm2_present()')
gate=b.index('g_firmware_trust!=FIRMWARE_TRUST_VERIFIED')
prepare=b.index('nova_tpm2_prepare_rollback_counter')
assert present < gate < prepare, 'firmware gate must precede persistent NV prepare/provisioning'
print('Stage-4C TPM persistent-state firmware gate ordering passed.')
PY
