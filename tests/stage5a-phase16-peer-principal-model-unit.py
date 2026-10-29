#!/usr/bin/env python3
from dataclasses import dataclass

UI=1<<0
TEXT=1<<1
TAB=1<<2
CB_READ=1<<3
CB_WRITE=1<<4
KNOWN=UI|TEXT|TAB|CB_READ|CB_WRITE

@dataclass
class App:
    app_id: str
    client_id: int
    pid: int
    generation: int
    declared: int
    effective: int

class Authority:
    def __init__(self, compositor_pid: int, apps, policy_authorized=True):
        self.compositor_pid=compositor_pid
        self.apps=list(apps)
        self.policy_authorized=policy_authorized
    def lookup(self, caller_pid: int, process_id: int):
        if not self.policy_authorized or caller_pid != self.compositor_pid or process_id <= 0:
            return None
        for app in self.apps:
            if app.pid != process_id:
                continue
            if not app.app_id or app.client_id <= 0 or app.generation <= 0:
                return None
            if app.declared & ~KNOWN or app.effective & ~KNOWN:
                return None
            if app.effective & ~app.declared:
                return None
            return app
        return None

class Compositor:
    def __init__(self, authority: Authority, pid=500):
        self.authority=authority
        self.pid=pid
        self.sessions={}
    def hello(self, peer_pid: int, claimed_client_id: int):
        principal=self.authority.lookup(self.pid, peer_pid)
        if principal is None:
            return False
        if not (principal.effective & UI):
            return False
        if principal.client_id != claimed_client_id:
            return False
        if principal.client_id in self.sessions:
            return False
        self.sessions[principal.client_id]=(peer_pid,principal.generation,principal.app_id,principal.effective)
        return True
    def disconnect(self, client_id: int):
        self.sessions.pop(client_id,None)

full=KNOWN
a=App('nova.demo.a',1,1001,1,full,full)
b=App('nova.demo.b',2,1002,1,full,full)
auth=Authority(500,[a,b])
comp=Compositor(auth)

assert comp.hello(1001,1)
assert not comp.hello(1002,1), 'App-B must not spoof App-A client_id'
assert not comp.hello(7777,2), 'unsupervised PID must fail closed'
assert not Authority(999,[a]).lookup(500,1001), 'only supervised compositor may query principals'

comp.disconnect(1)
a_no_ui=App('nova.demo.a',1,1001,1,full,full & ~UI)
auth.apps=[a_no_ui,b]
assert not comp.hello(1001,1), 'ui.surface is required to establish a surface session'

a_bad_subset=App('nova.demo.a',1,1001,1,UI,UI|CB_READ)
auth.apps=[a_bad_subset,b]
assert not comp.hello(1001,1), 'effective capabilities may not exceed declared set'

a_unknown=App('nova.demo.a',1,1001,1,full|(1<<40),full)
auth.apps=[a_unknown,b]
assert not comp.hello(1001,1), 'unknown capability bits fail closed'

auth.policy_authorized=False
auth.apps=[a,b]
assert not comp.hello(1001,1), 'principal lookup is denied outside authorized boot policy'
auth.policy_authorized=True

# Crash/relaunch: stale PID loses identity; new PID gets the same app/client identity
# with a strictly newer lifecycle generation.
old_pid=a.pid
a.pid=2001
a.generation=2
auth.apps=[a,b]
assert not comp.hello(old_pid,1), 'stale PID must not authenticate after relaunch'
assert comp.hello(2001,1)
assert comp.sessions[1][1] == 2

# Duplicate live session for one authenticated client is rejected even with the
# correct peer PID; disconnecting allows a later reconnect.
assert not comp.hello(2001,1)
comp.disconnect(1)
assert comp.hello(2001,1)

print('Stage-5A Phase-16 peer-PID/client-id spoof rejection + crash-generation rebinding model unit: PASS')
