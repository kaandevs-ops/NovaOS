#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
for n in esp a b recovery data; do printf 'nova-%s\n' "$n" > "$TMP/$n.bin"; done
python3 - "$TMP" <<'PY'
import json, sys
from pathlib import Path
p=Path(sys.argv[1]); M=1024*1024
spec={"partitions":[
 {"name":"NOVA_ESP","type":"esp","size_bytes":M,"payload":str(p/'esp.bin')},
 {"name":"NOVA_SYSTEM_A","type":"linux","size_bytes":M,"payload":str(p/'a.bin')},
 {"name":"NOVA_SYSTEM_B","type":"linux","size_bytes":M,"payload":str(p/'b.bin')},
 {"name":"NOVA_RECOVERY","type":"linux","size_bytes":M,"payload":str(p/'recovery.bin')},
 {"name":"NOVA_DATA","type":"linux","size_bytes":M,"payload":str(p/'data.bin')},
]}
(p/'spec.json').write_text(json.dumps(spec))
PY
python3 "$ROOT/scripts/storage/create-gpt-disk.py" build "$TMP/disk.img" "$TMP/spec.json" >/dev/null
OUT="$(python3 "$ROOT/scripts/storage/create-gpt-disk.py" inspect "$TMP/disk.img")"
for name in NOVA_ESP NOVA_SYSTEM_A NOVA_SYSTEM_B NOVA_RECOVERY NOVA_DATA; do grep -Fq "$name" <<<"$OUT"; done
grep -Fq 'GPT verification passed' <<<"$OUT"
python3 - "$TMP/disk.img.json" <<'PY'
import json,sys
m=json.load(open(sys.argv[1])); assert m['format']=='nova-gpt-v1'; assert len(m['partitions'])==5
starts=[p['first_lba'] for p in m['partitions']]; assert all(x%2048==0 for x in starts)
assert len({p['part_guid'] for p in m['partitions']})==5
PY
echo "GPT layout generation/CRC/alignment test passed."
