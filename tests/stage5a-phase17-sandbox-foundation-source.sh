#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAIN="$ROOT/src/init/main.c"
LINUX="$ROOT/src/runtime/nova_linux.h"
SYSCALL="$ROOT/src/runtime/nova_syscall.h"
POLICY="$ROOT/src/runtime/nova_app_sandbox.h"

for f in "$MAIN" "$LINUX" "$SYSCALL" "$POLICY"; do
  [[ -f "$f" ]]
done

# Bounded named policy: applications do not get raw namespace/seccomp authority.
grep -q 'NOVA_APP_SANDBOX_PROFILE_DESKTOP_V1_NAME "desktop-v1"' "$POLICY"
grep -q 'NOVA_APP_SANDBOX_FLAG_NO_NEW_PRIVS' "$POLICY"
grep -q 'NOVA_APP_SANDBOX_FLAG_MOUNT_NAMESPACE' "$POLICY"
grep -q 'NOVA_APP_SANDBOX_FLAG_PRIVATE_MOUNTS' "$POLICY"
! grep -qE 'mount_path|syscall_list|seccomp_program|bpf_program' "$POLICY"

# Raw Linux ABI is explicit for both supported architectures.
grep -q 'NOVA_CLONE_NEWNS = 0x00020000' "$LINUX"
grep -q 'NOVA_PR_SET_NO_NEW_PRIVS = 38' "$LINUX"
grep -q 'NOVA_MS_PRIVATE = 262144' "$LINUX"
grep -q 'NOVA_MS_REC = 16384' "$LINUX"
[[ "$(grep -c '^#define NOVA_NR_unshare' "$LINUX")" -eq 2 ]]
[[ "$(grep -c '^#define NOVA_NR_prctl' "$LINUX")" -eq 2 ]]
grep -q 'static inline long nova_unshare' "$SYSCALL"
grep -q 'static inline long nova_prctl' "$SYSCALL"

# Kernel foundations remain explicit and frozen-on for both future namespace and
# syscall-filter enforcement.
for marker in CONFIG_NAMESPACES=y CONFIG_SECCOMP=y CONFIG_SECCOMP_FILTER=y; do
  grep -qx "$marker" "$ROOT/configs/kernel/common.config"
done

# Production applications and Phase-16 application acceptance fixtures must
# remain launchable under the new mandatory sandbox declaration.
for manifest in \
  "$ROOT/configs/services/demo-app.service" \
  "$ROOT/configs/services/demo-app-b.service" \
  "$ROOT/tests/fixtures/stage5a-phase16/security-probe-read.service" \
  "$ROOT/tests/fixtures/stage5a-phase16/security-probe-no-ui.service" \
  "$ROOT/tests/fixtures/stage5a-phase16/security-probe-write.service"; do
  grep -qx 'kind=application' "$manifest"
  [[ "$(grep -c '^sandbox=desktop-v1$' "$manifest")" -eq 1 ]]
done
! grep -q '^sandbox=' "$ROOT/tests/fixtures/stage5a-phase16/security-probe-unregistered.service"

python3 - "$MAIN" <<'PY'
import re, sys
text=open(sys.argv[1], encoding='utf-8').read()

def body(name):
    m=re.search(r'\b'+re.escape(name)+r'\s*\([^;]*?\)\s*\{', text, re.S)
    if not m:
        raise SystemExit(f'missing function: {name}')
    start=m.end(); depth=1; i=start
    while i < len(text) and depth:
        if text[i]=='{': depth+=1
        elif text[i]=='}': depth-=1
        i+=1
    if depth:
        raise SystemExit(f'unbalanced function: {name}')
    return text[start:i-1]

parse=body('parse_manifest')
for marker in (
    'nova_streq(key,"sandbox")',
    'sandbox_seen',
    'application_sandbox_profile_from_name',
    '!sandbox_seen',
    '|| sandbox_seen',
    'application_sandbox_policy_valid',
):
    if marker not in parse:
        raise SystemExit(f'missing fail-closed manifest marker: {marker}')

apply=body('apply_application_sandbox')
sequence=[
    'nova_unshare(NOVA_CLONE_NEWNS)',
    'NOVA_MS_REC|NOVA_MS_PRIVATE',
    'nova_prctl(NOVA_PR_SET_NO_NEW_PRIVS,1UL,0UL,0UL,0UL)',
]
pos=-1
for marker in sequence:
    n=apply.find(marker)
    if n < 0 or n <= pos:
        raise SystemExit(f'bad sandbox setup ordering: {marker}')
    pos=n

start=body('start_service')
apply_pos=start.find('apply_application_sandbox(svc)')
exec_pos=start.find('nova_execve(svc->exec,argv,envp)')
exit126=start.find('nova_exit(126)')
if min(apply_pos, exec_pos, exit126) < 0:
    raise SystemExit('missing pre-exec sandbox fail-closed path')
if not (apply_pos < exit126 < exec_pos):
    raise SystemExit('application sandbox must fail before execve')
if 'svc->is_application && !apply_application_sandbox(svc)' not in start:
    raise SystemExit('system services must retain existing launch path')
PY

echo 'Stage-5A Phase-17 trusted sandbox-profile + pre-exec namespace/no-new-privs source contract: PASS'
