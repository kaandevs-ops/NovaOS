#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

while IFS= read -r -d '' file; do
  bash -n "$file"
done < <(find "$ROOT/scripts" "$ROOT/tests" -type f -name '*.sh' -print0 | sort -z)

python3 - "$ROOT" <<'PY'
from pathlib import Path
import re, sys
root=Path(sys.argv[1])
files=sorted([*root.joinpath('scripts').rglob('*.py'), *root.joinpath('tests').rglob('*.py')])
for path in files:
    source=path.read_text(encoding='utf-8')
    compile(source, str(path), 'exec', dont_inherit=True)
print(f'Python source syntax: PASS ({len(files)} files)')

# `bash -n` cannot detect a runtime nounset trap caused by dependent assignments
# inside one `local` builtin invocation, e.g. `local dir=... envfile="$dir/x"`.
# Bash expands every assignment word before all new locals from that builtin are
# reliably available to subsequent assignment words. Under `set -u`, that turns
# into an unbound-variable failure. Keep path derivations in separate commands.
violations=[]
for path in sorted([*root.joinpath('scripts').rglob('*.sh'), *root.joinpath('tests').rglob('*.sh')]):
    for lineno, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
        if not re.match(r'^\s*local\s+', line):
            continue
        assigns=list(re.finditer(r'(?<!\S)([A-Za-z_][A-Za-z0-9_]*)=', line))
        for m in assigns:
            name=m.group(1)
            tail=line[m.end():]
            if re.search(r'\$(?:'+re.escape(name)+r'\b|\{'+re.escape(name)+r'\})', tail):
                violations.append(f'{path.relative_to(root)}:{lineno}: local {name}=... is referenced in the same local command')
                break
if violations:
    print('Shell local-assignment nounset hazards:', file=sys.stderr)
    for item in violations:
        print('  '+item, file=sys.stderr)
    raise SystemExit(1)
print('Shell local-assignment nounset audit: PASS')
PY

echo "Shell source syntax: PASS"
