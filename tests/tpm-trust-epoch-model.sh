#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TPM="$ROOT/src/runtime/nova_tpm2.c"
python3 - <<'PY'
# Stage-4D logical model: TPM trust counter N means minimum accepted epoch N+1.
def accepts(counter, policy_epoch):
    minimum=counter+1
    return minimum <= policy_epoch <= minimum+1
assert accepts(0,1)
assert accepts(0,2)       # one-step pending rotation
assert not accepts(0,3)   # skipped future epoch
assert not accepts(1,1)   # stale signed policy after commit
assert accepts(1,2)
# Power-loss cases: either rollback or trust TPM commit may land first.  The
# confirmed target remains recoverable because each state is at most one step ahead.
for rollback_counter, trust_counter in ((0,0),(1,0),(0,1),(1,1)):
    target_rb=1; target_epoch=2
    assert target_rb in (rollback_counter, rollback_counter+1)
    assert accepts(trust_counter,target_epoch)
print('Stage-4D TPM trust-epoch monotonic/power-loss model passed.')
PY

# Logical monotonic counter exhaustion must fail closed before an NV increment.
grep -Fq 'before!=expected_current || before==~0UL' "$TPM"
