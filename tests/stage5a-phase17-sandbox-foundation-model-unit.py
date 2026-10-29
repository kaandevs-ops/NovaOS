#!/usr/bin/env python3
"""Phase-17 sandbox profile and fail-before-exec model."""

DESKTOP = "desktop-v1"
NO_NEW_PRIVS = 1 << 0
MOUNT_NAMESPACE = 1 << 1
PRIVATE_MOUNTS = 1 << 2
PRIVATE_ROOT = 1 << 3
PRIVATE_TMP = 1 << 4
APP_DATA = 1 << 5
DROP_PRIVS = 1 << 6
CLOSE_FDS = 1 << 7
PRIVATE_RUNTIME_IPC = 1 << 8
SECCOMP_DESKTOP = 1 << 9
DESKTOP_FLAGS = (NO_NEW_PRIVS | MOUNT_NAMESPACE | PRIVATE_MOUNTS | PRIVATE_ROOT |
                 PRIVATE_TMP | APP_DATA | DROP_PRIVS | CLOSE_FDS |
                 PRIVATE_RUNTIME_IPC | SECCOMP_DESKTOP)


def parse(kind, sandbox_values):
    if len(sandbox_values) > 1:
        return None
    if kind == "application":
        if sandbox_values != [DESKTOP]:
            return None
        return (1, DESKTOP_FLAGS)
    if kind == "service":
        if sandbox_values:
            return None
        return (0, 0)
    return None


def launch(profile, kernel_results):
    if profile != (1, DESKTOP_FLAGS):
        return "DENY"
    ordered = ["unshare", "private_mounts", "private_root", "no_new_privs",
               "close_fds", "drop_privileges", "seccomp"]
    completed = []
    for operation in ordered:
        if not kernel_results.get(operation, False):
            return ("DENY", tuple(completed), "execve=NO")
        completed.append(operation)
    return ("ALLOW", tuple(completed), "execve=YES")


assert parse("application", [DESKTOP]) == (1, DESKTOP_FLAGS)
assert parse("application", []) is None
assert parse("application", ["none"]) is None
assert parse("application", ["custom"]) is None
assert parse("application", [DESKTOP, DESKTOP]) is None
assert parse("service", []) == (0, 0)
assert parse("service", [DESKTOP]) is None

ok = {k: True for k in ("unshare", "private_mounts", "private_root", "no_new_privs",
                         "close_fds", "drop_privileges", "seccomp")}
assert launch((1, DESKTOP_FLAGS), ok)[0] == "ALLOW"
for failed in ok:
    vector = dict(ok); vector[failed] = False
    outcome = launch((1, DESKTOP_FLAGS), vector)
    assert outcome[0] == "DENY" and outcome[2] == "execve=NO"

assert launch((0, 0), ok) == "DENY"
assert launch((1, DESKTOP_FLAGS | (1 << 12)), ok) == "DENY"
print("Stage-5A Phase-17 canonical sandbox profile + fail-before-exec model unit: PASS")
