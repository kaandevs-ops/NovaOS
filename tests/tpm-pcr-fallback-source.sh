#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/runtime/nova_tpm2.c"
grep -Fq 'read_pcr_sha256_direct(pcr,hex_out)' "$SRC"
grep -Fq 'read_pcr_sha256_sysfs(pcr,hex_out)' "$SRC"
grep -Fq '/sys/class/tpm/tpm0/pcr-sha256/' "$SRC"
grep -Fq 'if(read_pcr_sha256_sysfs(pcr,hex_out)) return 1;' "$SRC"
grep -Fq 'return read_pcr_sha256_direct(pcr,hex_out);' "$SRC"
# Extend must remain a real TPM command, never a sysfs write.
grep -Fq 'TPM2_CC_PCR_EXTEND' "$SRC"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
r=s[s.index('int nova_tpm2_read_pcr_sha256(unsigned int pcr'):]
assert r.index('read_pcr_sha256_sysfs') < r.index('read_pcr_sha256_direct')
path=s[s.index('static int build_pcr_sysfs_path'):s.index('static int read_pcr_sha256_sysfs')]
assert 'append_dec_u32' in path and 'pcr>23U' in path
print('Stage-4C stable-kernel-PCR-primary + direct-TPM fallback policy passed.')
PY
