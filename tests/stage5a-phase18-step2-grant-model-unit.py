#!/usr/bin/env python3
"""Pure policy model; does not substitute for C binary / guest runtime evidence."""
from dataclasses import dataclass

CAP_RESOURCE_DEMO_READ = 1 << 5
KNOWN_MASK = (1 << 6) - 1

@dataclass(frozen=True)
class Principal:
    pid: int
    app_id: str
    client_id: int
    generation: int
    policy_generation: int
    declared: int
    effective: int

def broker_grant(auth_at_accept, live):
    if not auth_at_accept or not live:
        return "DENIED", 0
    for p in (auth_at_accept, live):
        if not p.pid or not p.app_id or p.client_id <= 0 or p.generation <= 0:
            return "DENIED", 0
        if p.policy_generation <= 0 or p.effective & ~p.declared:
            return "DENIED", 0
        if p.effective & ~KNOWN_MASK:
            return "DENIED", 0
        if not (p.effective & CAP_RESOURCE_DEMO_READ):
            return "DENIED", 0
    if (auth_at_accept.pid, auth_at_accept.app_id, auth_at_accept.client_id,
            auth_at_accept.generation, auth_at_accept.policy_generation) != (
            live.pid, live.app_id, live.client_id, live.generation, live.policy_generation):
        return "DENIED", 0
    return "GRANT_READONLY_FD", 1

p = Principal(101, "nova.resource.probe.grant", 5, 17, 1, 32, 32)
assert broker_grant(p, p) == ("GRANT_READONLY_FD", 1)
assert broker_grant(None, p) == ("DENIED", 0)
assert broker_grant(p, None) == ("DENIED", 0)
assert broker_grant(p, Principal(101, p.app_id, 5, 18, 1, 32, 32)) == ("DENIED", 0)
assert broker_grant(p, Principal(102, p.app_id, 5, 17, 1, 32, 32)) == ("DENIED", 0)
assert broker_grant(p, Principal(101, p.app_id, 5, 17, 2, 32, 32)) == ("DENIED", 0)
assert broker_grant(p, Principal(101, p.app_id, 6, 17, 1, 32, 32)) == ("DENIED", 0)
assert broker_grant(p, Principal(101, "nova.other", 5, 17, 1, 32, 32)) == ("DENIED", 0)
assert broker_grant(p, Principal(101, p.app_id, 5, 17, 1, 0, 0)) == ("DENIED", 0)
assert broker_grant(p, Principal(101, p.app_id, 5, 17, 1, 0, 32)) == ("DENIED", 0)
assert broker_grant(p, Principal(101, p.app_id, 5, 17, 1, 96, 96)) == ("DENIED", 0)
assert broker_grant(Principal(101, p.app_id, 5, 17, 1, 0, 0), p) == ("DENIED", 0)
print("Phase-18 Step-2 live-principal generation/policy/permission grant model unit: PASS")
