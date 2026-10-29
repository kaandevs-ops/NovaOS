#!/usr/bin/env python3
"""Phase-13 graceful process-stop policy model; not QEMU runtime acceptance."""

STOPPED=1
RUNNING=3
STOPPING=4
RESTARTING=5

class App:
    def __init__(self):
        self.pid=101
        self.generation=1
        self.state=RUNNING
        self.desired_running=True
        self.restart_always=True
        self.signal_stage=0
        self.signals=[]

    def stop_intent(self):
        if self.pid<=0:
            self.desired_running=False
            self.state=STOPPED
            return 'not-running'
        self.desired_running=False
        if self.state!=STOPPING:
            self.state=STOPPING
            self.signal_stage=0
        return 'graceful-pending'

    def timeout(self):
        assert self.state==STOPPING and self.pid>0
        if self.signal_stage==0:
            self.signals.append('TERM')
            self.signal_stage=1
        elif self.signal_stage==1:
            self.signals.append('KILL')
            self.signal_stage=2

    def exit(self):
        self.pid=0
        self.signal_stage=0
        if self.desired_running and self.restart_always:
            self.state=RESTARTING
            self.generation+=1
            self.pid=100+self.generation
            self.state=RUNNING
            return 'restarted'
        self.state=STOPPED
        return 'stopped'

    def launch(self):
        assert self.state==STOPPED and self.pid==0
        self.desired_running=True
        self.generation+=1
        self.pid=100+self.generation
        self.state=RUNNING

# User close: STOP intent happens before compositor close and does not signal.
a=App()
old_pid=a.pid
assert a.stop_intent()=='graceful-pending'
assert a.state==STOPPING
assert not a.desired_running
assert a.signals==[]

# App receives CLOSE_REQUEST for all toplevels, cleans resources and exits 0.
assert a.exit()=='stopped'
assert a.state==STOPPED and a.pid==0
assert a.generation==1
assert a.signals==[]

# Relaunch is a fresh process incarnation.
a.launch()
assert a.state==RUNNING and a.pid!=old_pid and a.generation==2

# An unresponsive app is escalated only after graceful intent.
b=App()
assert b.stop_intent()=='graceful-pending'
assert b.signals==[]
b.timeout()
assert b.signals==['TERM'] and b.state==STOPPING
b.timeout()
assert b.signals==['TERM','KILL'] and b.state==STOPPING
assert b.exit()=='stopped'
assert b.state==STOPPED and not b.desired_running

# Unexpected crash while desired_running remains true still follows restart policy.
c=App()
old_generation=c.generation
assert c.exit()=='restarted'
assert c.state==RUNNING
assert c.desired_running
assert c.generation==old_generation+1

print('Stage-5A Phase-13 graceful application stop/reap/escalation state-machine model unit: PASS')
