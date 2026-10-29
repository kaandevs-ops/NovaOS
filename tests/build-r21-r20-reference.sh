#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OLD="$(dirname "$ROOT")/phase19-desktop-r20-refinement-candidate"
clang -std=c11 -O2 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I"$OLD/src/runtime" -I"$OLD/files/src" \
 "$ROOT/tests/r21-r20-reference-compositor.c" "$OLD/tests/r17-fixture-authority.c" \
 "$OLD/files/src/nova_files_core.c" "$OLD/files/src/nova_files_ui.c" "$OLD/files/src/nova_files_font.c" \
 "$OLD/src/runtime/nova_runtime.c" "$OLD/src/runtime/nova_font.c" -Wl,--gc-sections -o "$ROOT/build/x86_64/r17-native/test-r20-reference"

clang -std=c11 -O2 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I"$ROOT/src/runtime" -I"$OLD/files/src" \
 "$ROOT/tests/r21-legacy-clients-compositor.c" "$OLD/tests/r17-fixture-authority.c" "$OLD/files/src/nova_files_core.c" "$OLD/files/src/nova_files_ui.c" "$OLD/files/src/nova_files_font.c" "$ROOT/src/runtime/nova_runtime.c" "$ROOT/src/runtime/nova_font.c" -Wl,--gc-sections -o "$ROOT/build/x86_64/r17-native/test-legacy-clients"

OLD19="$(dirname "$ROOT")/phase19-desktop-r19-premium-design-candidate"
clang -std=c11 -O2 -g -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I"$OLD19/src/runtime" -I"$OLD19/files/src" \
 "$ROOT/tests/r21-r19-profile-compositor.c" "$OLD19/tests/r17-fixture-authority.c" "$OLD19/files/src/nova_files_core.c" "$OLD19/files/src/nova_files_ui.c" "$OLD19/files/src/nova_files_font.c" "$OLD19/src/runtime/nova_runtime.c" "$OLD19/src/runtime/nova_font.c" -Wl,--gc-sections -o "$ROOT/build/x86_64/r17-native/test-r19-profile"
