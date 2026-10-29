#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
P="$ROOT/scripts/windows/stage5a-phase1-check.py"
python3 - "$P" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
assert "TPM_SOCK=Path('/tmp')/f'nova5a-swtpm-{os.getuid()}-{os.getpid()}.sock'" in s
assert "TPM_SOCK=TPM_DIR/'swtpm-sock'" not in s
assert "AF_UNIX socket paths are limited" in s
assert "--- swtpm log ---" in s
# Regression guard: the old nested checkout path was 128 bytes and exceeded
# Linux sockaddr_un.sun_path.  The runtime socket expression must stay short.
sample='/tmp/nova5a-swtpm-1000-123456.sock'
assert len(sample.encode()) < 100
print('Stage-5A short AF_UNIX swtpm control-socket path regression: PASS')
PY
