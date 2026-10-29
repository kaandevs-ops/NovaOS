#!/usr/bin/env python3

"""
Deterministic model for Phase-13 novad application lifecycle policy.

This is a model/unit test only.
It does not claim QEMU/OVMF runtime coverage.
"""

from dataclasses import dataclass


STOPPED = 1
STARTING = 2
RUNNING = 3
STOPPING = 4
RESTARTING = 5
FAILED = 6

OK = 0
UNKNOWN_APP = 1
ALREADY_RUNNING = 2
NOT_RUNNING = 3
SPAWN_FAILED = 4


REGISTRY = {
    "nova.demo.a": "/System/Apps/nova-demo-app",
    "nova.demo.b": "/System/Apps/nova-demo-app-b",
}


@dataclass
class App:
    app_id: str
    state: int = STOPPED
    pid: int = 0
    generation: int = 0
    desired_running: bool = False
    restart_always: bool = True


class Manager:
    def __init__(self):
        self.apps = {
            app_id: App(app_id)
            for app_id in REGISTRY
        }
        self.next_pid = 100

    def _spawn(self, app: App, fail: bool = False):
        app.state = STARTING

        if fail:
            app.pid = 0
            app.desired_running = False
            app.state = FAILED
            return SPAWN_FAILED

        self.next_pid += 1
        app.pid = self.next_pid
        app.generation += 1
        app.state = RUNNING
        return OK

    def launch(self, app_id: str, fail: bool = False):
        app = self.apps.get(app_id)

        if app is None:
            return UNKNOWN_APP

        if app.pid > 0 or app.state in (
            STARTING,
            RUNNING,
            STOPPING,
            RESTARTING,
        ):
            return ALREADY_RUNNING

        app.desired_running = True
        return self._spawn(app, fail=fail)

    def stop(self, app_id: str):
        app = self.apps.get(app_id)

        if app is None:
            return UNKNOWN_APP

        if app.pid <= 0:
            app.desired_running = False
            app.state = STOPPED
            return NOT_RUNNING

        app.desired_running = False
        app.state = STOPPING
        return OK

    def child_exit(self, app_id: str, *, restart_fail: bool = False):
        app = self.apps[app_id]
        app.pid = 0

        if app.desired_running and app.restart_always:
            app.state = RESTARTING
            return self._spawn(app, fail=restart_fail)

        app.state = STOPPED
        return OK


m = Manager()

assert m.launch("/bin/sh") == UNKNOWN_APP
assert m.launch("nova.unknown") == UNKNOWN_APP

assert m.launch("nova.demo.a") == OK

a = m.apps["nova.demo.a"]
first_pid = a.pid
first_generation = a.generation

assert a.state == RUNNING
assert a.desired_running
assert first_pid > 0
assert first_generation == 1

assert m.launch("nova.demo.a") == ALREADY_RUNNING
assert (a.pid, a.generation) == (first_pid, first_generation)

assert m.stop("nova.demo.a") == OK
assert a.state == STOPPING
assert not a.desired_running

assert m.child_exit("nova.demo.a") == OK
assert a.state == STOPPED
assert a.pid == 0
assert a.generation == first_generation

assert m.stop("nova.demo.a") == NOT_RUNNING

assert m.launch("nova.demo.a") == OK
assert a.state == RUNNING
assert a.pid != first_pid
assert a.generation == first_generation + 1

pre_crash_pid = a.pid
pre_crash_generation = a.generation

assert m.child_exit("nova.demo.a") == OK
assert a.state == RUNNING
assert a.pid != pre_crash_pid
assert a.generation == pre_crash_generation + 1

b = m.apps["nova.demo.b"]

assert m.launch("nova.demo.b") == OK
immediate_pid = b.pid

assert m.child_exit("nova.demo.b") == OK
assert b.state == RUNNING
assert b.pid != immediate_pid
assert b.generation == 2

m2 = Manager()

assert m2.launch("nova.demo.b", fail=True) == SPAWN_FAILED

b2 = m2.apps["nova.demo.b"]

assert b2.state == FAILED
assert b2.pid == 0
assert not b2.desired_running

print(
    "Stage-5A Phase-13 novad application lifecycle "
    "state-machine model unit: PASS"
)
