#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIXTURE_SCRIPT="$ROOT/scripts/build-stage4d-rotation-fixtures.sh"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

MOCKROOT="$TMP/mockroot"
FIX="$TMP/fix"
OUT="$TMP/out"
mkdir -p "$MOCKROOT/scripts/verity" "$MOCKROOT/scripts" "$TMP/bin" "$FIX" \
  "$OUT/rootfs/System/Trust" "$OUT/rootfs/System/Integrity" "$OUT/rootfs/System/Core"
# R24 build cleanup is retained; this mock dependency is confined to this private fixture.
cat > "$TMP/mockroot/scripts/retain-build-files.py" <<'RETAIN_HELPER'
from pathlib import Path
import sys,uuid
scope=Path(__file__).resolve().parents[2];out=scope/'retained';out.mkdir(exist_ok=True)
for arg in sys.argv[1:]:
 if arg.startswith('-'):continue
 p=Path(arg)
 if not p.exists():continue
 assert p.resolve().is_relative_to(scope) and p.resolve()!=scope
 p.rename(out/(uuid.uuid4().hex+'-'+p.name))
RETAIN_HELPER
printf 'format=3\ntrust_epoch=2\nactive_key_id=key-b\n' > "$OUT/rootfs/System/Trust/release-policy.conf"
printf 'policy-sig\n' > "$OUT/rootfs/System/Trust/release-policy.sig"
printf 'active-cert\n' > "$OUT/rootfs/System/Trust/release-active.der"
printf 'payload\n' > "$OUT/rootfs/System/Core/payload"
printf 'development-unit-key\n' > "$TMP/root.key"
cp "$ROOT/scripts/generate-integrity-manifest.py" "$MOCKROOT/scripts/generate-integrity-manifest.py"
chmod +x "$MOCKROOT/scripts/generate-integrity-manifest.py"

cat > "$MOCKROOT/scripts/verity/build-system-slot.sh" <<'MOCKVERITY'
#!/usr/bin/env bash
set -euo pipefail
src="$1"; out="$2"; meta="$6"
mkdir -p "$(dirname "$out")" "$(dirname "$meta")"
rm -rf "$out.source"
cp -a "$src" "$out.source"
printf 'slot' > "$out"
cat > "$meta" <<META
payload_sha256=$(printf slot | sha256sum | awk '{print $1}')
payload_size=4
verity_data_block_size=4096
verity_hash_block_size=4096
verity_data_blocks=1
verity_hash_start_block=1
verity_hash_algorithm=sha256
verity_root_hash=$(printf root | sha256sum | awk '{print $1}')
verity_salt=$(printf salt | sha256sum | awk '{print $1}')
META
MOCKVERITY
chmod +x "$MOCKROOT/scripts/verity/build-system-slot.sh"

cat > "$TMP/bin/openssl" <<'MOCKOPENSSL'
#!/usr/bin/env bash
set -euo pipefail
out=''
while (($#)); do
  if [[ "$1" == -out ]]; then out="$2"; shift 2; else shift; fi
done
[[ -n "$out" ]]
printf 'sig' > "$out"
MOCKOPENSSL
chmod +x "$TMP/bin/openssl"

# Execute the production helper functions, not a reimplementation.  This catches
# rootfs/System layout regressions that shell syntax and source grep cannot.
{
  echo 'set -euo pipefail'
  printf 'ROOT=%q\n' "$MOCKROOT"
  printf 'FIX=%q\n' "$FIX"
  printf 'OUT=%q\n' "$OUT"
  printf 'ROOT_KEY=%q\n' "$TMP/root.key"
  awk '/^build_system_fixture\(\) \{/{emit=1} emit{print} emit && /^}$/{exit}' "$FIXTURE_SCRIPT"
  awk '/^build_negative_system_fixture\(\) \{/{emit=1} emit{print} emit && /^}$/{exit}' "$FIXTURE_SCRIPT"
  echo "build_negative_system_fixture corrupt-policy corrupt-policy id-corrupt version 1 salt-corrupt"
  echo "build_negative_system_fixture missing-active-key missing-active-key id-missing version 1 salt-missing"
} > "$TMP/run.sh"
PATH="$TMP/bin:$PATH" bash "$TMP/run.sh"

for name in corrupt-policy missing-active-key; do
  for f in system.slot system.env verity.manifest verity.sig; do
    [[ -s "$FIX/$name/$f" ]] || { echo "missing runtime-unit fixture $name/$f" >&2; exit 1; }
  done
  [[ -s "$FIX/$name/system.slot.source/Integrity/manifest.sha256" ]] || { echo "missing System integrity manifest snapshot for $name" >&2; exit 1; }
  grep -Fq '  /System/' "$FIX/$name/system.slot.source/Integrity/manifest.sha256" || { echo "manifest has no /System entries for $name" >&2; exit 1; }
done

grep -Fq '# controlled detached-policy corruption' "$FIX/corrupt-policy/system.slot.source/Trust/release-policy.conf"
[[ ! -e "$FIX/missing-active-key/system.slot.source/Trust/release-active.der" ]]

echo "Stage-4D negative fixture rootfs/System runtime layout unit: PASS"
