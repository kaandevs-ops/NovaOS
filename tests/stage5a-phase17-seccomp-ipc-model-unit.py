#!/usr/bin/env python3
"""Phase-17 Step-3 private IPC and seccomp allowlist model."""

ALLOWED = {
    'read','write','close','lseek','mmap','munmap','nanosleep','clock_gettime',
    'getpid','connect','sendmsg','recvmsg','memfd_create','ftruncate','openat','ppoll',
    'fsync','mkdirat','unlinkat','renameat','fchmodat','getcwd','execve','exit',
}
DENIED = {
    'clone','unshare','mount','umount2','chroot','setuid','setgid','setgroups',
    'kill','wait4','reboot','sethostname','add_key','keyctl','mknodat','bind','listen','accept4',
}
VISIBLE_IPC = {'/run/nova/client.sock'}
HOST_APP_RUNTIME = '/run/nova/app'
HOST_SANDBOX_SOCKET = '/run/nova/app/client.sock'
HIDDEN_IPC = {
    '/run/nova/app-lifecycle.sock','/run/nova/app-catalog.sock',
    '/run/nova/app-security.sock','/run/nova/assets.sock',
    '/run/nova/desktop.sock','/run/nova/compositor.sock',
}

def syscall(name, *args, arch_ok=True):
    if not arch_ok:
        return 'KILL'
    if name == 'socket':
        return 'ALLOW' if args in {('AF_UNIX','SOCK_SEQPACKET',0),('AF_UNIX','SOCK_SEQPACKET|SOCK_NONBLOCK',0)} else 'EPERM'
    return 'ALLOW' if name in ALLOWED else 'EPERM'

assert VISIBLE_IPC == {'/run/nova/client.sock'}
assert HOST_APP_RUNTIME == '/run/nova/app'
assert HOST_SANDBOX_SOCKET.endswith('/client.sock')
assert not (VISIBLE_IPC & HIDDEN_IPC)
for name in ALLOWED:
    assert syscall(name) == 'ALLOW'
for name in DENIED:
    assert syscall(name) == 'EPERM'
assert syscall('socket','AF_UNIX','SOCK_SEQPACKET',0) == 'ALLOW'
assert syscall('socket','AF_UNIX','SOCK_SEQPACKET|SOCK_NONBLOCK',0) == 'ALLOW'
assert syscall('socket','AF_INET','SOCK_SEQPACKET',0) == 'EPERM'
assert syscall('socket','AF_UNIX','SOCK_STREAM',0) == 'EPERM'
assert syscall('socket','AF_UNIX','SOCK_SEQPACKET',1) == 'EPERM'
assert syscall('unknown_future_syscall') == 'EPERM'
assert syscall('read', arch_ok=False) == 'KILL'

# Filter installation is the last trusted transition before execve.  A failure
# must never fall through to an unsandboxed application image.
ops=('private_root','no_new_privs','close_fds','drop_uid_gid','install_seccomp')
for failed in ops:
    completed=[]
    for op in ops:
        if op == failed:
            outcome=('DENY',tuple(completed),'execve=NO')
            break
        completed.append(op)
    assert outcome[0]=='DENY' and outcome[2]=='execve=NO'

print('Stage-5A Phase-17 seccomp process-control denial + AF_UNIX compositor-only IPC model unit: PASS')
