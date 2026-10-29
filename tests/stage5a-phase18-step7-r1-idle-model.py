#!/usr/bin/env python3
"""State-machine vectors only; separate guest QEMU required for acceptance."""
MAX=4
IDLE=120

class Sim:
    def __init__(self):
        self.slots=[None]*MAX;self.disconnects=0;self.expirations=0
    def connect(self,now):
        for i,v in enumerate(self.slots):
            if v is None:
                self.slots[i]=now;return i
        return None
    def valid_request(self,slot,now):
        assert self.slots[slot] is not None
        self.slots[slot]=now
    def invalid_request(self,slot,now):
        assert self.slots[slot] is not None
    def close(self,slot):
        if self.slots[slot] is not None:
            self.slots[slot]=None;self.disconnects+=1
    def expire(self,now,clock_ok=True):
        if not clock_ok:
            for i in range(MAX): self.close(i)
            return
        for i,last in enumerate(self.slots):
            if last is not None and now>=last and now-last>=IDLE:
                self.close(i);self.expirations+=1

m=Sim()
assert [m.connect(10) for _ in range(4)] == [0,1,2,3]
assert m.connect(11) is None
m.valid_request(0,100)
m.invalid_request(1,110)
m.expire(130)
assert m.slots==[100,None,None,None]
assert m.expirations==3 and m.disconnects==3
assert m.connect(130)==1
m.valid_request(1,240)
m.expire(249)
assert m.slots==[None,240,None,None]
assert m.expirations==4
m.expire(360)
assert m.slots==[None]*MAX and m.expirations==5 and m.disconnects==5
assert m.connect(361)==0
m.expire(362,clock_ok=False)
assert m.slots==[None]*MAX and m.disconnects==6 and m.expirations==5
print('Phase-18 Step-7 R1 capacity/valid-activity/invalid-request/120s/clock-fail model: PASS (not QEMU)')
