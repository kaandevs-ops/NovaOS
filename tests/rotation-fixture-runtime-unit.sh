#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIXTURE_SCRIPT="$ROOT/scripts/build-stage4d-rotation-fixtures.sh"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$TMP/mockroot/scripts/verity" "$TMP/bin" "$TMP/fix" "$TMP/source/System"
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
printf 'fixture-source\n' > "$TMP/source/System/file"
printf 'development-unit-key\n' > "$TMP/root.key"

cat > "$TMP/mockroot/scripts/verity/build-system-slot.sh" <<'MOCKVERITY'
#!/usr/bin/env bash
set -euo pipefail
out="$2"; meta="$6"
mkdir -p "$(dirname "$out")" "$(dirname "$meta")"
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
chmod +x "$TMP/mockroot/scripts/verity/build-system-slot.sh"

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

# Execute the actual function body from the production fixture builder. This is
# specifically a runtime nounset test; bash -n cannot detect dependent-local
# expansion failures such as `local dir=... envfile="$dir/..."`.
{
  echo 'set -euo pipefail'
  printf 'ROOT=%q\n' "$TMP/mockroot"
  printf 'FIX=%q\n' "$TMP/fix"
  printf 'ROOT_KEY=%q\n' "$TMP/root.key"
  awk '/^build_system_fixture\(\) \{/{emit=1} emit{print} emit && /^}$/{exit}' "$FIXTURE_SCRIPT"
  printf 'build_system_fixture unit %q id version 1 salt-text\n' "$TMP/source/System"
  printf 'test -s %q\n' "$TMP/fix/unit/system.slot"
  printf 'test -s %q\n' "$TMP/fix/unit/system.env"
  printf 'test -s %q\n' "$TMP/fix/unit/verity.manifest"
  printf 'test -s %q\n' "$TMP/fix/unit/verity.sig"
} > "$TMP/run.sh"

PATH="$TMP/bin:$PATH" bash "$TMP/run.sh"
echo "Stage-4D rotation fixture function runtime nounset unit: PASS"
