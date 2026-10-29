#!/usr/bin/env bash
set -euo pipefail
BASE="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
TASK="$(mktemp -d)"
echo "R16_TEST_ARTIFACTS=$TASK"
CC="${CC:-clang}"
"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -I"$BASE/src" -I"$BASE/reference" "$BASE/tests/test_ui.c" "$BASE/src/nova_files_ui.c" \
 "$BASE/src/nova_files_core.c" "$BASE/src/nova_files_linux_host.c" "$BASE/src/nova_files_font.c" -o "$TASK/r15-test-ui"
"$CC" -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 -I"$BASE/src" -I"$BASE/reference" "$BASE/tests/test_surface_bridge.c" "$BASE/src/nova_files_surface_bridge.c" \
 "$BASE/src/nova_files_ui.c" "$BASE/src/nova_files_core.c" "$BASE/src/nova_files_linux_host.c" \
 "$BASE/src/nova_files_font.c" -o "$TASK/r15-test-surface"
"$TASK/r15-test-ui"
"$TASK/r15-test-surface"
"$CC" -std=c11 -O2 -Wall -Wextra -Werror -I"$BASE/src" -I"$BASE/reference" \
 "$BASE/tools/nova_files_host_preview.c" "$BASE/src/nova_files_ui.c" \
 "$BASE/src/nova_files_core.c" "$BASE/src/nova_files_linux_host.c" "$BASE/src/nova_files_font.c" -o "$TASK/r15-preview"
mkdir -p "$TASK/documents" "$TASK/projects"
printf 'NovaOS R15\n' > "$TASK/readme.txt"
"$TASK/r15-preview" "$TASK" "$TASK/snapshot.ppm"
test "$(head -n1 "$TASK/snapshot.ppm")" = P6
for file in nova_files_core.c nova_files_ui.c nova_files_surface_bridge.c; do
  "$CC" -std=c11 -ffreestanding -fno-builtin -Wall -Wextra -Werror -fsyntax-only -I"$BASE/src" -I"$BASE/reference" "$BASE/src/$file"
done
"$CC" -std=c11 -ffreestanding -fno-builtin -Wall -Wextra -Werror -fsyntax-only -I"$BASE/src" "$BASE/src/nova_files_font.c"
if command -v clang >/dev/null 2>&1; then
 for file in nova_files_core.c nova_files_ui.c nova_files_surface_bridge.c; do
  clang --target=aarch64-linux-gnu -std=c11 -ffreestanding -fno-builtin -Wall -Wextra -Werror -fsyntax-only -I"$BASE/src" -I"$BASE/reference" "$BASE/src/$file"
 done
 echo 'R15_ARM64_FREESTANDING=PASS'
else echo 'R15_ARM64_FREESTANDING=SKIPPED_no_cross_compiler'; fi
echo 'R15_NATIVE_VISUAL_ENGINE=PASS'
echo 'R15_LOCAL_TESTS=PASS'
