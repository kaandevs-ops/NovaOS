#!/usr/bin/env python3
"""Phase-17 Step-2 private filesystem and credential-drop model."""
from dataclasses import dataclass

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

@dataclass(frozen=True)
class App:
    app_id: str
    client_id: int
    executable: str


def data_source(app: App) -> str:
    assert app.app_id and '/' not in app.app_id
    return f'/Data/Nova/Apps/{app.app_id}'


def sandbox_view(app: App):
    if not app.executable.startswith('/System/Apps/') or app.client_id <= 0:
        return None
    return {
        app.executable: ('bind-ro-exec', app.executable),
        '/Data/App': ('bind-rw-noexec', data_source(app)),
        '/tmp': ('private-tmpfs-noexec', None),
        '/run/nova': ('bind-ro-app-runtime-dir', '/run/nova/app'),
        '/run/nova/client.sock': ('dynamic-sandbox-socket', '/run/nova/app/client.sock'),
    }


def launch(app: App, results):
    ordered = [
        'prepare_data', 'unshare', 'private_mounts', 'private_root',
        'no_new_privs', 'close_fds', 'setgroups', 'setgid', 'setuid',
    ]
    done=[]
    for op in ordered:
        if not results.get(op, False):
            return ('DENY', tuple(done), 'execve=NO')
        done.append(op)
    return ('ALLOW', tuple(done), 'execve=YES', app.client_id, app.client_id)


a = App('nova.demo.a', 1, '/System/Apps/nova-demo-app')
b = App('nova.demo.b', 2, '/System/Apps/nova-demo-app-b')
assert DESKTOP_FLAGS == 0x3ff
assert data_source(a) != data_source(b)
view = sandbox_view(a)
assert view is not None
assert view['/Data/App'][1] == '/Data/Nova/Apps/nova.demo.a'
assert view['/tmp'][0] == 'private-tmpfs-noexec'
assert view[a.executable][0] == 'bind-ro-exec'

# Sensitive host trees are absent from the constructed chroot namespace.
for hidden in ('/Data/Nova', '/Data/Users', '/System/Trust', '/System/Integrity',
               '/System/Services', '/Recovery', '/proc', '/sys', '/dev'):
    assert hidden not in view

# Only the application's own executable is projected from /System/Apps.
assert '/System/Apps/nova-demo-app-b' not in view

ok={op: True for op in ('prepare_data','unshare','private_mounts','private_root',
                        'no_new_privs','close_fds','setgroups','setgid','setuid')}
out=launch(a,ok)
assert out[0]=='ALLOW' and out[-2:]==(1,1)
for failed in ok:
    v=dict(ok); v[failed]=False
    denied=launch(a,v)
    assert denied[0]=='DENY' and denied[2]=='execve=NO'

assert sandbox_view(App('bad', 0, '/System/Apps/x')) is None
assert sandbox_view(App('bad', 1, '/System/Core/x')) is None
print('Stage-5A Phase-17 private filesystem visibility + unique unprivileged identity fail-closed model unit: PASS')
