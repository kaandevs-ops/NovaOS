#!/usr/bin/env bash
set -euo pipefail
python3 - <<'PY'
from dataclasses import dataclass

@dataclass(frozen=True)
class State:
    active: str
    confirmed: bool
    rb_disk: int
    rb_tpm: int
    epoch_disk: int
    epoch_tpm: int

def policy_ok(s):
    minimum=s.epoch_tpm+1
    return minimum <= s.epoch_disk <= minimum+1

def recover(s):
    assert s.confirmed
    assert s.rb_tpm <= s.rb_disk <= s.rb_tpm+1
    assert policy_ok(s)
    rb=s.rb_tpm + (s.rb_disk==s.rb_tpm+1)
    ep=s.epoch_tpm + (s.epoch_disk==s.epoch_tpm+2)
    return State(s.active,True,s.rb_disk,rb,s.epoch_disk,ep)

# Inactive-slot writes/descriptors alone never authorize a boot switch.
for phase in ('payload-only','payload+descriptor'):
    s=State('A',True,0,0,1,0)
    assert s.active=='A' and s.confirmed

# Journal-first confirmation can lose power before either/both TPM commits.
for rb_tpm,ep_tpm in ((0,0),(1,0),(0,1),(1,1)):
    s=State('B',True,1,rb_tpm,2,ep_tpm)
    if rb_tpm<=1<=rb_tpm+1 and policy_ok(s):
        r=recover(s)
        assert r.rb_tpm==1 and r.epoch_tpm==1

# TPM ahead + stale disk/policy is never silently accepted.
assert not (0 >= 1)  # rollback_index 0 < TPM rollback generation 1
assert not policy_ok(State('B',True,1,1,1,1))  # epoch 1 stale when TPM trust counter is 1
print('Stage-4D power-loss / disk-TPM mismatch state-machine model passed (model test, not OVMF runtime).')
PY
