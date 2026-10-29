#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
L="$ROOT/src/runtime/nova_linux.h"; S="$ROOT/src/runtime/nova_syscall.h"
for marker in \
  'struct nova_iovec' 'struct nova_msghdr' 'struct nova_cmsghdr' \
  'NOVA_CMSG_SPACE(sizeof(int)) == 24' 'NOVA_SOL_SOCKET = 1' 'NOVA_SCM_RIGHTS = 1' \
  'NOVA_MFD_CLOEXEC = 0x0001' 'NOVA_MSG_CTRUNC = 0x08' 'NOVA_MSG_TRUNC = 0x20' 'NOVA_MSG_CMSG_CLOEXEC = 0x40000000'; do
  grep -Fq "$marker" "$L" || { echo "missing Phase-6 Linux UAPI marker: $marker" >&2; exit 1; }
done
for marker in \
  '#define NOVA_NR_sendmsg       46' '#define NOVA_NR_recvmsg       47' '#define NOVA_NR_ftruncate     77' '#define NOVA_NR_memfd_create  319' '#define NOVA_NR_lseek         8' \
  '#define NOVA_NR_sendmsg       211' '#define NOVA_NR_recvmsg       212' '#define NOVA_NR_ftruncate     46' '#define NOVA_NR_memfd_create  279' '#define NOVA_NR_lseek         62'; do
  grep -Fq "$marker" "$L" || { echo "missing Phase-6 syscall ABI marker: $marker" >&2; exit 1; }
done
for fn in nova_sendmsg nova_recvmsg nova_memfd_create nova_ftruncate nova_lseek; do grep -Fq "${fn}(" "$S"; done
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
cat >"$TMP/abi.c" <<'SRC'
#include "nova_linux.h"
#include "nova_surface_protocol.h"
int phase6_abi(void) { return (int)(sizeof(struct nova_msghdr)+sizeof(struct nova_surface_message)); }
SRC
for target in x86_64-linux-gnu aarch64-linux-gnu; do
  clang --target="$target" -ffreestanding -fno-builtin -nostdlib -Wall -Wextra -Werror -I"$ROOT/src/runtime" -c "$TMP/abi.c" -o "$TMP/${target}.o"
done
echo 'Stage-5A Phase-6 memfd/sendmsg/recvmsg/ftruncate/lseek + ancillary-data ABI contract: PASS'
