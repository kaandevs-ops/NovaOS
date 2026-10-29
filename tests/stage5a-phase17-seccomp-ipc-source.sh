#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAIN="$ROOT/src/init/main.c"
LINUX="$ROOT/src/runtime/nova_linux.h"
POLICY="$ROOT/src/runtime/nova_app_sandbox.h"
SURFACE="$ROOT/src/runtime/nova_surface_protocol.h"
COMPOSITOR="$ROOT/src/services/compositor.c"

for f in "$MAIN" "$LINUX" "$POLICY" "$SURFACE" "$COMPOSITOR"; do [[ -f "$f" ]]; done

for marker in \
  NOVA_APP_SANDBOX_FLAG_PRIVATE_RUNTIME_IPC \
  NOVA_APP_SANDBOX_FLAG_SECCOMP_DESKTOP_V1 \
  'NOVA_APP_SANDBOX_RUNTIME_DIR "/run/nova"' \
  'NOVA_APP_SANDBOX_COMPOSITOR_SOCKET "/run/nova/client.sock"'; do
  grep -q "$marker" "$POLICY"
done

for marker in \
  'NOVA_PR_SET_SECCOMP = 22' \
  'NOVA_SECCOMP_MODE_FILTER = 2' \
  NOVA_SECCOMP_RET_KILL_PROCESS \
  NOVA_SECCOMP_RET_ERRNO \
  NOVA_SECCOMP_RET_ALLOW \
  NOVA_SECCOMP_DATA_ARCH_OFFSET \
  NOVA_SECCOMP_DATA_ARG0_OFFSET \
  NOVA_AUDIT_ARCH_NATIVE; do
  grep -q "$marker" "$LINUX"
done

grep -qx 'CONFIG_SECCOMP=y' "$ROOT/configs/kernel/common.config"
grep -qx 'CONFIG_SECCOMP_FILTER=y' "$ROOT/configs/kernel/common.config"

# Legacy ABI remains frozen while compositor exposes a dedicated dynamic app-only
# directory for sandbox bind-mounts.
grep -Fq 'NOVA_SURFACE_SOCKET_PATH "/run/nova/client.sock"' "$SURFACE"
grep -Fq 'NOVA_SURFACE_SANDBOX_SOCKET_DIR "/run/nova/app"' "$SURFACE"
grep -Fq 'NOVA_SURFACE_SANDBOX_SOCKET_PATH "/run/nova/app/client.sock"' "$SURFACE"
grep -q 'setup_sandbox_client_listener' "$COMPOSITOR"
grep -q 'NOVA_SURFACE_SANDBOX_SOCKET_PATH' "$COMPOSITOR"
grep -q 'sandbox_client_ipc=LISTENING' "$COMPOSITOR"
grep -q 'accept_client_connection(sandbox_client_listener,app_clients)' "$COMPOSITOR"

python3 - "$MAIN" <<'PY'
import re, sys
text=open(sys.argv[1], encoding='utf-8').read()

def body(name):
    m=re.search(r'\b'+re.escape(name)+r'\s*\([^;]*?\)\s*\{', text, re.S)
    if not m: raise SystemExit(f'missing function: {name}')
    start=m.end(); depth=1; i=start
    while i < len(text) and depth:
        if text[i]=='{': depth+=1
        elif text[i]=='}': depth-=1
        i+=1
    if depth: raise SystemExit(f'unbalanced function: {name}')
    return text[start:i-1]

root=body('sandbox_build_private_root')
for marker in (
    'NOVA_APP_SANDBOX_FLAG_PRIVATE_RUNTIME_IPC',
    'sandbox_bind_mount(NOVA_SURFACE_SANDBOX_SOCKET_DIR,path,NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC)',
):
    if marker not in root: raise SystemExit(f'missing private runtime marker: {marker}')
if 'sandbox_bind_mount("/run/nova"' in root:
    raise SystemExit('host /run/nova must not be projected wholesale into applications')
for forbidden in ('app-lifecycle.sock','app-catalog.sock','app-security.sock','assets.sock','desktop.sock','compositor.sock'):
    if forbidden in root: raise SystemExit(f'forbidden runtime IPC projection: {forbidden}')

sec=body('sandbox_install_desktop_seccomp')
ordered=[
    'NOVA_SECCOMP_DATA_ARCH_OFFSET',
    'NOVA_AUDIT_ARCH_NATIVE',
    'NOVA_SECCOMP_RET_KILL_PROCESS',
    'NOVA_SECCOMP_DATA_NR_OFFSET',
    'NOVA_NR_socket',
    'NOVA_SECCOMP_DATA_ARG0_OFFSET',
    'NOVA_AF_UNIX',
    'NOVA_SECCOMP_DATA_ARG1_OFFSET',
    'NOVA_SOCK_SEQPACKET',
    'NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK',
    'NOVA_SECCOMP_DATA_ARG2_OFFSET',
    'NOVA_SECCOMP_RET_ALLOW',
]
pos=-1
for marker in ordered:
    n=sec.find(marker)
    if n<0 or n<=pos: raise SystemExit(f'bad seccomp/socket ordering: {marker}')
    pos=n

required_allow=(
    'read','write','close','lseek','mmap','munmap','nanosleep','clock_gettime',
    'getpid','connect','sendmsg','recvmsg','memfd_create','ftruncate','openat','ppoll',
    'fsync','mkdirat','unlinkat','renameat','fchmodat','getcwd','execve','exit'
)
for name in required_allow:
    marker=f'NOVA_SECCOMP_ALLOW_NR(NOVA_NR_{name})'
    if marker not in sec: raise SystemExit(f'missing app syscall allow: {name}')

for name in ('clone','unshare','mount','umount2','chroot','setuid','setgid','setgroups',
             'kill','wait4','reboot','sethostname','add_key','keyctl','mknodat'):
    if f'NOVA_SECCOMP_ALLOW_NR(NOVA_NR_{name})' in sec:
        raise SystemExit(f'dangerous syscall allowed by desktop filter: {name}')

if 'NOVA_SECCOMP_RET_ERRNO|NOVA_EPERM' not in sec:
    raise SystemExit('missing fail-closed EPERM default')
if 'nova_prctl(NOVA_PR_SET_SECCOMP,NOVA_SECCOMP_MODE_FILTER' not in sec:
    raise SystemExit('seccomp filter is not installed through prctl')

apply=body('apply_application_sandbox')
seq=[
    'nova_prctl(NOVA_PR_SET_NO_NEW_PRIVS,1UL,0UL,0UL,0UL)',
    'nova_close_range(3U,0xffffffffU,0U)',
    'sandbox_drop_application_privileges(svc)',
    'sandbox_install_desktop_seccomp()',
]
pos=-1
for marker in seq:
    n=apply.find(marker)
    if n<0 or n<=pos: raise SystemExit(f'bad seccomp pre-exec ordering: {marker}')
    pos=n

start=body('start_service')
apply_pos=start.find('apply_application_sandbox(svc)')
exec_pos=start.find('nova_execve(svc->exec,argv,envp)')
if min(apply_pos,exec_pos)<0 or apply_pos>=exec_pos:
    raise SystemExit('seccomp sandbox must be installed before application execve')
PY

echo 'Stage-5A Phase-17 private compositor-IPC view + bounded seccomp syscall/socket policy source contract: PASS'
