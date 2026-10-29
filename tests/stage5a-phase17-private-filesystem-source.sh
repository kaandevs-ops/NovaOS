#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAIN="$ROOT/src/init/main.c"
LINUX="$ROOT/src/runtime/nova_linux.h"
SYSCALL="$ROOT/src/runtime/nova_syscall.h"
POLICY="$ROOT/src/runtime/nova_app_sandbox.h"

for f in "$MAIN" "$LINUX" "$SYSCALL" "$POLICY"; do [[ -f "$f" ]]; done

# desktop-v1 is still a bounded trusted profile, but now expands to a real
# private root/data/tmp/credential boundary before execve.
for marker in \
  NOVA_APP_SANDBOX_FLAG_PRIVATE_ROOT \
  NOVA_APP_SANDBOX_FLAG_PRIVATE_TMP \
  NOVA_APP_SANDBOX_FLAG_APP_DATA \
  NOVA_APP_SANDBOX_FLAG_DROP_PRIVILEGES \
  NOVA_APP_SANDBOX_FLAG_CLOSE_INHERITED_FDS \
  'NOVA_APP_SANDBOX_ROOT "/.nova-sandbox-root"' \
  'NOVA_APP_DATA_ROOT "/Data/Nova/Apps"' \
  'NOVA_APP_SANDBOX_DATA_PATH "/Data/App"'; do
  grep -q "$marker" "$POLICY"
done

for nr in chroot chdir setuid setgid setgroups fchownat fchmodat close_range; do
  [[ "$(grep -c "^#define NOVA_NR_${nr}" "$LINUX")" -eq 2 ]]
done
for fn in nova_chroot nova_chdir nova_setuid nova_setgid nova_setgroups nova_fchownat nova_fchmodat nova_close_range; do
  grep -q "static inline long ${fn}" "$SYSCALL"
done
grep -qx 'CONFIG_TMPFS=y' "$ROOT/configs/kernel/common.config"

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

prepare=body('sandbox_prepare_persistent_data')
for marker in ('NOVA_APP_DATA_ROOT','sandbox_app_data_path','nova_fchownat','nova_fchmodat'):
    if marker not in prepare: raise SystemExit(f'missing persistent data marker: {marker}')

root=body('sandbox_build_private_root')
ordered=[
    'string_has_prefix(svc->exec,"/System/Apps/")',
    'nova_mount("tmpfs",NOVA_APP_SANDBOX_ROOT',
    'sandbox_bind_mount(svc->exec,exec_target,NOVA_MS_RDONLY',
    'sandbox_bind_mount(app_data,path,NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC)',
    'nova_mount("tmpfs",path,"tmpfs",NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC,"mode=1777,size=16m")',
    'sandbox_bind_mount(NOVA_SURFACE_SANDBOX_SOCKET_DIR,path,NOVA_MS_RDONLY|NOVA_MS_NOSUID|NOVA_MS_NODEV|NOVA_MS_NOEXEC)',
    'nova_chroot(NOVA_APP_SANDBOX_ROOT)',
    'nova_chdir("/")',
]
pos=-1
for marker in ordered:
    n=root.find(marker)
    if n < 0 or n <= pos: raise SystemExit(f'bad private-root ordering: {marker}')
    pos=n

# The chroot is constructed from explicit mounts; forbidden host trees are not
# projected into the app root.
for forbidden in ('"/proc"','"/sys"','"/dev"','"/Recovery"','"/System/Trust"','"/System/Integrity"','"/System/Services"'):
    if forbidden in root: raise SystemExit(f'forbidden sandbox projection: {forbidden}')

priv=body('sandbox_drop_application_privileges')
seq=['nova_setgroups(0','nova_setgid(svc->client_id)','nova_setuid(svc->client_id)']
pos=-1
for marker in seq:
    n=priv.find(marker)
    if n<0 or n<=pos: raise SystemExit(f'bad privilege-drop ordering: {marker}')
    pos=n

apply=body('apply_application_sandbox')
seq=[
    'nova_unshare(NOVA_CLONE_NEWNS)',
    'NOVA_MS_REC|NOVA_MS_PRIVATE',
    'sandbox_build_private_root(svc)',
    'nova_prctl(NOVA_PR_SET_NO_NEW_PRIVS,1UL,0UL,0UL,0UL)',
    'nova_close_range(3U,0xffffffffU,0U)',
    'sandbox_drop_application_privileges(svc)',
    'sandbox_install_desktop_seccomp()',
]
pos=-1
for marker in seq:
    n=apply.find(marker)
    if n<0 or n<=pos: raise SystemExit(f'bad sandbox enforcement ordering: {marker}')
    pos=n

start=body('start_service')
prepare_pos=start.find('sandbox_prepare_persistent_data(svc)')
clone_pos=start.find('nova_clone_process()')
apply_pos=start.find('apply_application_sandbox(svc)')
exec_pos=start.find('nova_execve(svc->exec,argv,envp)')
if min(prepare_pos,clone_pos,apply_pos,exec_pos)<0 or not (prepare_pos < clone_pos < apply_pos < exec_pos):
    raise SystemExit('persistent data + pre-exec sandbox ordering is not fail-closed')
PY

echo 'Stage-5A Phase-17 private chroot/data/tmp + inherited-FD purge + UID/GID drop source contract: PASS'
