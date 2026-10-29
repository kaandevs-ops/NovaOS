#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
L="$ROOT/src/runtime/nova_linux.h"; S="$ROOT/src/runtime/nova_syscall.h"; P="$ROOT/src/runtime/nova_shell_protocol.h"
grep -Fq '#define NOVA_NR_socket        41' "$L"
grep -Fq '#define NOVA_NR_accept4       288' "$L"
grep -Fq '#define NOVA_NR_socket        198' "$L"
grep -Fq '#define NOVA_NR_accept4       242' "$L"
grep -Fq 'NOVA_SOCK_SEQPACKET = 5' "$L"
grep -Fq 'sizeof(struct nova_sockaddr_un) == 110' "$L"
grep -Fq 'static inline long nova_socket' "$S"
grep -Fq 'static inline long nova_bind' "$S"
grep -Fq 'static inline long nova_accept4' "$S"
grep -Fq 'static inline long nova_connect' "$S"
grep -Fq '#define NOVA_SHELL_PROTOCOL_VERSION 1U' "$P"
grep -Fq 'sizeof(struct nova_shell_message) == 48' "$P"
grep -Fq 'NOVA_SHELL_OP_SCENE' "$P"
echo 'Stage-5A Phase-4 Unix-domain IPC + versioned shell protocol UAPI contract: PASS'
