#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; SRC="$ROOT/src/runtime/nova_tpm2.c"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
b=s[s.index('int nova_tpm2_present(void)'):s.index('typedef struct {')]
assert 'nova_open_rw("/dev/tpmrm0")' in b
assert '/sys/class/tpm/tpm0/tpm_version_major' in b
assert 'nova_open_rw("/dev/tpm0")' in b
assert b.index('nova_open_rw("/dev/tpmrm0")') < b.index('/sys/class/tpm/tpm0/tpm_version_major')
# TPM presence must not be sysfs-only: a TPM2 resource-manager node is sufficient.
assert 'if(fd>=0){ nova_close((int)fd); return 1; }' in b
print('Stage-4C TPM2 presence is resource-manager-first with sysfs/raw fallback.')
PY
