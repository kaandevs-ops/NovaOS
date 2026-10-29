#!/usr/bin/env python3

"""Phase-13 cold-launch policy model; not QEMU/OVMF runtime acceptance."""

from dataclasses import dataclass

STOPPED=1
RUNNING=3
OK=0
ALREADY_RUNNING=2

@dataclass
class App:
    app_id: str
    enabled: bool = True
    autostart: bool = False
    pid: int = 0
    state: int = STOPPED
    generation: int = 0
    desired_running: bool = False

apps=[App('nova.demo.a'),App('nova.demo.b')]

# Boot starts no application process when autostart=no.
next_pid=200
for app in apps:
    if app.enabled and app.autostart:
        raise AssertionError('non-autostart application was boot-started')
assert all(app.pid==0 and app.state==STOPPED and app.generation==0 for app in apps)

# Launcher request produces the first real process incarnation.
def launch(app):
    global next_pid
    if app.pid>0:
        return ALREADY_RUNNING
    app.desired_running=True
    next_pid+=1
    app.pid=next_pid
    app.generation+=1
    app.state=RUNNING
    return OK

assert launch(apps[0])==OK
assert apps[0].pid>0 and apps[0].generation==1 and apps[0].desired_running
assert apps[1].pid==0 and apps[1].generation==0

# Duplicate launch does not fork another process.
pid=apps[0].pid
gen=apps[0].generation
assert launch(apps[0])==ALREADY_RUNNING
assert (apps[0].pid,apps[0].generation)==(pid,gen)

print('Stage-5A Phase-13 non-autostart cold-launch policy model unit: PASS')
