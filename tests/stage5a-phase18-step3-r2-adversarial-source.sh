#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
B="$ROOT/src/services/resource_broker.c"
P="$ROOT/tests/fixtures/stage5a-phase18/resource_probe.c"
U="$ROOT/scripts/build-userland.sh"
for x in 'step3_create_fixture_objects' 'NOVA_STEP3_TEST_FIFO' 'NOVA_STEP3_TEST_OVERSIZE' 'g_step3_type_denials' 'g_step3_size_denials' 'NOVA_PHASE18_STEP3_ADVERSARIAL'; do grep -Fq "$x" "$B" || exit 1; done
for x in 'step3-directory=PASS' 'step3-fifo=PASS' 'step3-oversize=PASS'; do grep -Fq "$x" "$P" || exit 1; done
grep -Fq 'STEP3_ADVERSARIAL' "$U"
for TARGET in x86_64-linux-gnu aarch64-linux-gnu; do
  clang --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only -I"$ROOT/src/runtime" -DNOVA_PHASE18_STEP3_ADVERSARIAL=1 "$B"
  for GRANTED in 0 1; do
    clang --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only -I"$ROOT/src/runtime" -DNOVA_PHASE18_STEP3_ADVERSARIAL=1 -DNOVA_RESOURCE_PROBE_GRANTED="$GRANTED" "$P"
  done
done
python3 - "$ROOT/scripts/windows/stage5a-phase18-step3-check.py" <<'PYSYNTAX'
import ast,sys
ast.parse(open(sys.argv[1],encoding="utf-8").read(),filename=sys.argv[1])
PYSYNTAX
echo 'Phase-18 Step-3 R2 test-only adversarial source/dual-arch syntax: PASS (not guest acceptance)'
