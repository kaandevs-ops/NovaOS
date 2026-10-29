#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH=x86_64
ARCHIVE="$ROOT/build/$ARCH/initramfs.cpio"
if [[ ! -f "$ARCHIVE" ]]; then
  "$ROOT/scripts/build-userland.sh" "$ARCH" >/dev/null
  "$ROOT/scripts/build-initramfs.sh" "$ARCH" >/dev/null
fi
python3 - "$ARCHIVE" <<'PY'
from pathlib import Path
import stat, sys
b=Path(sys.argv[1]).read_bytes(); pos=0; found=None
while pos < len(b):
    while pos < len(b) and b[pos] == 0:
        pos += 1
    if pos >= len(b): break
    if b[pos:pos+6] not in (b'070701', b'070702'):
        raise SystemExit(f'invalid initramfs member magic at offset {pos}')
    h=b[pos:pos+110]
    vals=[int(h[6+i*8:6+(i+1)*8],16) for i in range(13)]
    mode=vals[1]; size=vals[6]; rmaj=vals[9]; rmin=vals[10]; namesize=vals[11]
    pos += 110
    name=b[pos:pos+namesize-1].decode('utf-8',errors='strict')
    pos += namesize
    pos=(pos+3)&~3
    pos += size
    pos=(pos+3)&~3
    if name == 'dev/console':
        found=(mode,rmaj,rmin)
    # TRAILER!!! terminates only this cpio member; concatenated members follow.
if found is None:
    raise SystemExit('/dev/console missing from initramfs')
mode,rmaj,rmin=found
assert stat.S_ISCHR(mode), f'/dev/console is not a char device: mode={mode:o}'
assert (mode & 0o777) == 0o600, f'/dev/console permissions wrong: {mode & 0o777:o}'
assert (rmaj,rmin)==(5,1), f'/dev/console device wrong: {rmaj}:{rmin}'
print('Stage-4B embedded initramfs /dev/console node passed: char 5:1 mode 0600')
PY
