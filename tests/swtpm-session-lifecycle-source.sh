#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/scripts/windows/final-tpm-check.py"
for needle in \
  'def reset_swtpm_state()' \
  'def start_swtpm_instance(session_name)' \
  'def stop_swtpm_instance(proc)' \
  "swtpm_proc,swtpm_log=start_swtpm_instance(session_name)" \
  'stop_swtpm_instance(swtpm_proc)' \
  "'--tpmstate',f'dir={TPM_DIR}'" \
  "stat.S_ISSOCK(mode)"; do
  grep -Fq -- "$needle" "$P"
done
python3 - "$P" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
assert s.count('reset_swtpm_state()') >= 2  # definition + one top-level reset
assert s.count('start_swtpm_instance(session_name)') >= 3  # definition + session + rejection
assert 'if TPM_DIR.exists(): shutil.rmtree(TPM_DIR)' in s
# State reset is not allowed inside the per-session starter: persistent NV state
# must survive emulator process/socket restarts.
start=s.index('def start_swtpm_instance(session_name):')
stop=s.index('def stop_swtpm_instance(proc):', start)
body=s[start:stop]
assert 'shutil.rmtree(TPM_DIR)' not in body
# A single long-lived swtpm process is intentionally not used anymore.
assert 'swtpm_proc=start_swtpm()' not in s
print('Stage-4C per-QEMU swtpm lifecycle with persistent TPM state source contract passed.')
PY
