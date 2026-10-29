#!/usr/bin/env python3
"""Phase-13 shell launch migration policy model; not QEMU runtime acceptance."""

OK=0
ALREADY_RUNNING=2
UNKNOWN_APP=1
SPAWN_FAILED=4
DENIED=6
RUNNING=3
STARTING=2
FAILED=6


def launch_policy(result,state):
    if result==OK and state==RUNNING:
        return 'novad-spawn-no-compositor-launch'
    if result==ALREADY_RUNNING and state==RUNNING:
        return 'resident-window-fallback'
    return 'reject-no-compositor-launch'

assert launch_policy(OK,RUNNING)=='novad-spawn-no-compositor-launch'
assert launch_policy(ALREADY_RUNNING,RUNNING)=='resident-window-fallback'
assert launch_policy(ALREADY_RUNNING,STARTING)=='reject-no-compositor-launch'
assert launch_policy(UNKNOWN_APP,FAILED)=='reject-no-compositor-launch'
assert launch_policy(SPAWN_FAILED,FAILED)=='reject-no-compositor-launch'
assert launch_policy(DENIED,FAILED)=='reject-no-compositor-launch'

print('Stage-5A Phase-13 shell launch migration-policy model unit: PASS')
