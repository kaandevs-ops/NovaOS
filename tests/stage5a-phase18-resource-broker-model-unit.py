#!/usr/bin/env python3
"""Phase-18 Step-1 authenticated broker/no-grant-surface model."""
from dataclasses import dataclass

@dataclass(frozen=True)
class Principal:
    pid: int
    app_id: str
    client_id: int
    generation: int
    effective: int

class NovadAuthority:
    def __init__(self, broker_pid, live):
        self.broker_pid=broker_pid
        self.live={p.pid:p for p in live}
    def lookup(self, caller_pid, observed_app_pid):
        if caller_pid != self.broker_pid:
            return None
        p=self.live.get(observed_app_pid)
        if not p or p.generation <= 0:
            return None
        return p

class Broker:
    def __init__(self, pid, authority):
        self.pid=pid
        self.authority=authority
    def accept(self, kernel_peer_pid):
        return self.authority.lookup(self.pid, kernel_peer_pid)
    def request(self, session, request):
        # v1 request is framing only: no caller-selected identity or resource.
        assert set(request) == {'magic','version','opcode','size','request_id','reserved0','reserved1','reserved2'}
        if session is None:
            return ('DENIED', None)
        if request['opcode'] != 'HELLO':
            return ('UNSUPPORTED', None)
        return ('SESSION', session)

app=Principal(101,'nova.demo.a',1,7,0x1f)
auth=NovadAuthority(broker_pid=50,live=[app])
broker=Broker(50,auth)

assert auth.lookup(999,101) is None, 'only supervised broker may query resource authority'
assert broker.accept(101) == app
assert broker.accept(9999) is None, 'unregistered/stale peer PID must fail closed'

hello=dict(magic='NRBR',version=1,opcode='HELLO',size=32,request_id=1,reserved0=0,reserved1=0,reserved2=0)
kind,session=broker.request(broker.accept(101),hello)
assert kind=='SESSION' and session.app_id=='nova.demo.a' and session.client_id==1

unsupported=dict(hello); unsupported['opcode']='OPEN_FILE'
assert broker.request(app,unsupported) == ('UNSUPPORTED',None), 'Step-1 must not grant resources'

# A restarted app is rebound only through its new kernel PID/generation.
del auth.live[101]
restarted=Principal(202,'nova.demo.a',1,8,0x1f)
auth.live[202]=restarted
assert broker.accept(101) is None
assert broker.accept(202) == restarted

print('Stage-5A Phase-18 authenticated resource-broker session + no-grant-surface model unit: PASS')
