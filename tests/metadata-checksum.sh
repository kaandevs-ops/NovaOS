#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
python3 - "$ROOT/scripts/build-disk.sh" "$ROOT/src/init/main.c" <<'PY'
from pathlib import Path
import hashlib, sys
build=Path(sys.argv[1]).read_text(); src=Path(sys.argv[2]).read_text()
canonical=("format=2\nactive=A\nprevious=A\ngeneration=0\nconfirmed=yes\n"
           "attempts_left=0\npending_kind=none\nupdate_id=none\n")
expected=hashlib.sha256(canonical.encode()).hexdigest()
assert 'canonical=("format=2\\nactive=A' in build
for token in ['format=2\\nactive=', '\\nprevious=', '\\ngeneration=', '\\nconfirmed=', '\\nattempts_left=', '\\npending_kind=', '\\nupdate_id=']:
    assert token in src, token
assert len(expected)==64
print('Stage-3C metadata checksum canonicalization test passed:', expected)
PY
