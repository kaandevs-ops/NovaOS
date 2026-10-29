#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/scripts/windows/final-tpm-check.py"; S="$ROOT/scripts/windows/final-tpm-check.sh"
for needle in \
  "swtpm','socket'" \
  "--tpmstate" \
  "'-tpmdev','emulator,id=tpm0,chardev=chrtpm'" \
  "'-device','tpm-tis,tpmdev=tpm0'" \
  'hashlib.sha256(bytes.fromhex(vals[' \
  'Injected valid-checksum hostile boot-journal rollback' \
  'rollback_index=1 expected=2' \
  'antirollback_rejected=yes' \
  'TPM NV logical generation 0 -> 1 with arbitrary-raw write-locked baseline' \
  'secure-boot TPM/PCR preflight' \
  'dependent A/B boots were NOT executed' \
  'Live PCR7' \
  'tpm_device_node='; do grep -Fq -- "$needle" "$P"; done
for needle in 'run-stage4c-suite.sh' 'novaos-stage4c.img' 'CONFIG_TCG_TPM' 'swtpm' 'tpm2_pcrread'; do grep -Fq -- "$needle" "$S"; done
python3 - "$P" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
reset=s.index('reset_swtpm_state()')
pf=s.index("pf=run_session",reset)
reset2=s.index('reset_swtpm_state()',pf)
b1=s.index("b1=run_session",reset2); b2=s.index("b2=run_session",b1); b3=s.index("b3=run_session",b2)
inject=s.index('inject_hostile_journal_rollback()',b3); rb=s.index("r=run_session",inject); b5=s.index("b5=run_session",rb)
assert reset < pf < reset2 < b1 < b2 < b3 < inject < rb < b5
assert "TPM_DIR=OUT/'stage4c-swtpm'" in s
assert 'start_swtpm_instance(session_name)' in s
assert 'stop_swtpm_instance(swtpm_proc)' in s
assert 'fail_stateful(pf' in s and 'fail_stateful(b1' in s and 'fail_stateful(b2' in s and 'fail_stateful(b3' in s and 'fail_stateful(r' in s and 'fail_stateful(b5' in s
assert "cat /sys/class/tpm/tpm0/pcr-sha256/7" in s and "cat /sys/class/tpm/tpm0/pcr-sha256/14" in s
print('Stage-4C isolated TPM preflight + fail-fast stateful acceptance source ordering passed.')
PY
