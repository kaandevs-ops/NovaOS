#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; cd "$ROOT"
PY=scripts/windows/final-tpm-check.py
SH=scripts/windows/final-tpm-check.sh
grep -Fq "'swtpm_setup','--tpm2','--tpmstate'" "$PY"
grep -Fq "'--pcr-banks','sha256'" "$PY"
grep -Fq "TPM_SETUP_LOG.write_text" "$PY"
grep -Eq 'required=.*swtpm_setup' "$SH"
echo "Stage-4C swtpm manufacturing + explicit SHA-256 PCR bank passed."

# swtpm_setup and swtpm intentionally have different tpmstate syntaxes.
grep -Fq "'swtpm_setup','--tpm2','--tpmstate',str(TPM_DIR)" "$PY"
if grep -Fq "'swtpm_setup','--tpm2','--tpmstate',f'dir={TPM_DIR}'" "$PY"; then
  echo 'swtpm_setup must not use swtpm daemon dir= syntax' >&2
  exit 1
fi
