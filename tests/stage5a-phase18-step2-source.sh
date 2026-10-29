#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
N="$ROOT/src/init/main.c"
B="$ROOT/src/services/resource_broker.c"
H="$ROOT/src/runtime/nova_resource_broker_protocol.h"
S="$ROOT/src/runtime/nova_app_security_protocol.h"
A="$ROOT/scripts/build-initramfs.sh"
C="$ROOT/src/services/compositor.c"
for marker in 'NOVA_RESOURCE_BROKER_OP_OPEN_DEMO_RO' 'NOVA_RESOURCE_BROKER_OP_GRANT_FD' 'NOVA_RESOURCE_BROKER_FLAG_FD_READONLY' 'NOVA_RESOURCE_BROKER_DEMO_SOURCE'; do
  grep -Fq "$marker" "$H"
done
for marker in 'lookup_live_principal(session->peer_pid,&live)' 'live_session_matches(session,&live)' 'NOVA_APP_CAP_RESOURCE_DEMO_READ' 'nova_sendmsg(socket_fd,&msg,NOVA_MSG_NOSIGNAL)' 'NOVA_SCM_RIGHTS' 'NOVA_O_NOFOLLOW' 'NOVA_O_CLOEXEC' 'NOVA_O_RDONLY' 'NOVA_RESOURCE_BROKER_OP_OPEN_DEMO_RO' 'NOVA_RESOURCE_BROKER_RESULT_DENIED'; do
  grep -Fq "$marker" "$B"
done
for marker in 'NOVA_APP_CAP_RESOURCE_DEMO_READ' 'NOVA_APP_CAP_PHASE18_KNOWN_MASK' 'NOVA_APP_CAP_TOKEN_RESOURCE_DEMO_READ'; do grep -Fq "$marker" "$S"; done
for marker in 'NOVA_APP_CAP_TOKEN_RESOURCE_DEMO_READ' 'NOVA_APP_CAP_RESOURCE_DEMO_READ'; do grep -Fq "$marker" "$N"; done
grep -Fq 'NOVA_APP_CAP_PHASE18_KNOWN_MASK' "$C"
grep -Fq 'install -m 0444 "$ROOT/configs/resources/phase18-broker-demo.txt"' "$A"
! grep -q 'capabilities=.*resource.demo.read' "$ROOT/configs/services/demo-app.service"
! grep -q 'capabilities=.*resource.demo.read' "$ROOT/configs/services/demo-app-b.service"
for f in "$ROOT/tests/fixtures/stage5a-phase18/"*.service; do grep -qx 'sandbox=desktop-v1' "$f"; done
CC="${CC:-clang}"
DEFINES=(
  '-DNOVA_VERSION="0.0.12-dev"'
  '-DNOVA_BUILD_ARCH="test"'
  '-DNOVA_ROOT_CERT_SHA256="UNPINNED"'
  '-DNOVA_TRUST_PROFILE="development"'
  '-DNOVA_TRUST_KEY_ID="nova-development-root-2026"'
  '-DNOVA_UEFI_CERT_SHA256="UNPINNED"'
  '-DNOVA_RELEASE_KEY_ID="nova-development-release-2026-a"'
  -DNOVA_TRUST_EPOCH=1
)
for TARGET in x86_64-linux-gnu aarch64-linux-gnu; do
 "$CC" --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only -I"$ROOT/src/runtime" \
  "${DEFINES[@]}" \
  "$N" "$B" "$C" "$ROOT/tests/fixtures/stage5a-phase18/resource_probe.c" \
  -DNOVA_RESOURCE_PROBE_GRANTED=1
done
echo 'Phase-18 Step-2 brokered fixed read-only SCM_RIGHTS source/dual-arch syntax: PASS'
