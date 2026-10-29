#!/usr/bin/env python3
"""Step-6 R1 scheduler/capacity model, NOT guest acceptance or FD-runtime test."""
from collections import deque

MAX_CLIENTS=4

def tick(pending, live):
    # Mirrors the bounded accept_slots contract: no more than MAX_CLIENTS
    # accept4 operations before each active session receives a servicing turn.
    accepted=0
    authentication_calls=0
    rejected_full=0
    accepted_live=0
    for _ in range(MAX_CLIENTS):
        if not pending: break
        accepted+=1
        applicant=pending.popleft()
        if len(live)>=MAX_CLIENTS:
            rejected_full+=1  # accepted fd closed, no principal lookup
            continue
        authentication_calls+=1
        if applicant!='authorized':
            continue  # rejected, fd closed
        live.append(applicant)
        accepted_live+=1
    serviced=len(live)  # one process_session_packet opportunity per live fd
    return accepted,authentication_calls,rejected_full,accepted_live,serviced

q=deque(['unauthorized']*1000)
assert tick(q,[])==(4,4,0,0,0)
assert len(q)==996
q=deque(['authorized']*1000)
assert tick(q,[])==(4,4,0,4,4)
live=['authorized']*MAX_CLIENTS
before=len(q)
auth=0
for _ in range(100):
    accepted,lookups,full,created,serviced=tick(q,live)
    assert (accepted,lookups,full,created,serviced)==(4,0,4,0,4)
    auth+=lookups
assert auth==0 and before-len(q)==400
# After a disconnect, a later poll cycle can admit a newly authorized peer.
live.pop()
assert tick(deque(['authorized']),live)==(1,1,0,1,4)
# This model asserts scheduler bounds only: it cannot prove actual kernel
# FD cleanup, SO_PEERCRED, QEMU guest fairness, or restart behavior.
print('Phase-18 Step-6 R1 accept/capacity/fairness model unit: PASS (not QEMU)')
