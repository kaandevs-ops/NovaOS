#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TPM="$ROOT/src/runtime/nova_tpm2.c"
python3 - <<'PY'
# Independent model test for the TPM raw-counter -> NovaOS logical-generation mapping.
# TPM first raw counter value is device-specific and may be >1; NovaOS must remain 0,1,2...
def logical(counter_written, raw, baseline_written, baseline):
    if not counter_written:
        assert not baseline_written
        return 0
    assert baseline_written and raw >= baseline
    return 1 + (raw - baseline)

for first_raw in (1, 2, 17, 999, 2**32+7):
    # Freshly defined counter/base before first increment.
    assert logical(False, 0, False, 0) == 0
    # First confirmed update captures arbitrary raw counter as baseline.
    base = first_raw
    assert logical(True, first_raw, True, base) == 1
    # Subsequent raw increments map exactly to software generations.
    for n in range(1, 8):
        assert logical(True, first_raw + n - 1, True, base) == n

# Invalid/torn states must never be accepted as a generation.
for args in ((False,0,True,1),(True,4,True,5)):
    try:
        logical(*args)
    except AssertionError:
        pass
    else:
        raise SystemExit(f'invalid state accepted: {args!r}')
print('Stage-4C TPM NV arbitrary-raw baseline generation model passed.')
PY

# Logical monotonic counter exhaustion must fail closed before an NV increment.
grep -Fq 'before!=expected_current || before==~0UL' "$TPM"
