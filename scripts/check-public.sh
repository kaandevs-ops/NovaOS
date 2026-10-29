#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/public-check
clang -std=c11 -Wall -Wextra -Werror -Isrc/runtime tests/r49-keyboard.c -o build/public-check/keyboard
build/public-check/keyboard
gcc -std=c11 -Os -Wall -Wextra -Werror -Wno-misleading-indentation -fstack-protector-strong src/terminal/nova_terminal_session.c -o build/public-check/terminal-session
python3 - <<'PY'
import ast
from pathlib import Path
for p in Path('scripts').rglob('*.py'): ast.parse(p.read_text(),filename=str(p))
print('PYTHON_PARSE_PASS')
PY
for arch in x86_64-linux-gnu aarch64-linux-gnu; do
 for unit in compositor shell; do
  clang --target="$arch" -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -Os -Wall -Wextra -Werror -Isrc/runtime -c src/services/$unit.c -o build/public-check/$unit-$arch.o
 done
done
mkdir -p build/x86_64/r17-native
bash tests/r21-theme.sh
bash tests/r21-files-view.sh
echo PUBLIC_COMPONENT_CHECK_PASS
