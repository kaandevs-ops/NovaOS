#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BU="$ROOT/scripts/build-userland.sh"
BI="$ROOT/scripts/build-initramfs.sh"
FIX="$ROOT/tests/fixtures/stage5a-phase17"
PROBE="$FIX/sandbox_probe.c"

for f in sandbox_probe.c sandbox-probe-a.service sandbox-probe-b.service; do
  [[ -f "$FIX/$f" ]] || { echo "missing Phase-17 acceptance fixture: $f" >&2; exit 1; }
done
for f in sandbox-probe-a.service sandbox-probe-b.service; do
  grep -qx 'capabilities=ui.surface' "$FIX/$f"
  grep -qx 'sandbox=desktop-v1' "$FIX/$f"
  grep -qx 'kind=application' "$FIX/$f"
  grep -qx 'restart=always' "$FIX/$f"
done
grep -qx 'client_id=3' "$FIX/sandbox-probe-a.service"
grep -qx 'client_id=4' "$FIX/sandbox-probe-b.service"
grep -qx 'app_id=nova.sandbox.probe.a' "$FIX/sandbox-probe-a.service"
grep -qx 'app_id=nova.sandbox.probe.b' "$FIX/sandbox-probe-b.service"

# Acceptance-only probes must not enter ordinary production config.
! grep -q 'phase17-sandbox-probe' "$ROOT/configs/services/services.list"
for script in "$BU" "$BI"; do
  grep -Fq 'NOVA_PHASE17_ACCEPTANCE_PROBES' "$script"
  grep -Fq 'Phase-16 and Phase-17 acceptance probes are mutually exclusive' "$script"
done
for marker in nova-sandbox-probe-a nova-sandbox-probe-b; do grep -Fq "$marker" "$BU"; done
for marker in phase17-sandbox-probe-a.service phase17-sandbox-probe-b.service; do grep -Fq "$marker" "$BI"; done

for marker in \
  'PROBE_STATE_PATH "/Data/App/phase17-sandbox.state"' \
  'PROBE_PERSISTENT_PATH "/Data/App/persistent.marker"' \
  'persistent_previous_pid' \
  'read_pid_marker(PROBE_PERSISTENT_PATH)' \
  'write_pid_marker(PROBE_PERSISTENT_PATH,(unsigned long)nova_getpid())' \
  'PROBE_TMP_PATH "/tmp/phase17.marker"' \
  '"/System/Trust/nova-root.der"' \
  '"/System/Integrity/manifest.sha256"' \
  '"/System/Services/services.list"' \
  '"/Data/Nova"' \
  '"/Recovery/recovery.conf"' \
  '"/proc/self/status"' \
  '"/run/nova/app-security.sock"' \
  'nova_socket(NOVA_AF_INET,NOVA_SOCK_STREAM,0)' \
  'nova_socket(NOVA_AF_UNIX,NOVA_SOCK_STREAM,0)' \
  'nova_syscall0(9999L)==-NOVA_EPERM' \
  'nova_kill(nova_getpid(),0)==-NOVA_EPERM' \
  'nova_wait4(-1,&status,NOVA_WNOHANG)==-NOVA_EPERM' \
  'connect_path(NOVA_SURFACE_SOCKET_PATH,NOVA_SOCK_SEQPACKET|NOVA_SOCK_NONBLOCK' \
  'connect_attempts' \
  'connect_last_error' \
  'socket_last_error' \
  'write_state("CONNECTING")' \
  'g_probe.reconnects++'; do
  grep -Fq "$marker" "$PROBE" || { echo "missing Phase-17 runtime probe marker: $marker" >&2; exit 1; }
done

grep -Fq 'NOVA_AF_INET = 2' "$ROOT/src/runtime/nova_linux.h"

CC="${CC:-clang}"
for ARCH in x86_64 aarch64; do
  TARGET=x86_64-linux-gnu; [[ "$ARCH" == aarch64 ]] && TARGET=aarch64-linux-gnu
  for MODE in NOVA_SANDBOX_PROBE_A NOVA_SANDBOX_PROBE_B; do
    "$CC" --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only \
      -I"$ROOT/src/runtime" -D"$MODE"=1 "$PROBE"
  done
done

bash -n "$BU" "$BI"
echo 'Stage-5A Phase-17 acceptance-only private-filesystem/seccomp probe build wiring source contract: PASS'
