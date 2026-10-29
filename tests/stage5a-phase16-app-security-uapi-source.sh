#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HDR="$ROOT/src/runtime/nova_app_security_protocol.h"

[[ -f "$HDR" ]] || { echo 'FAIL: nova_app_security_protocol.h missing' >&2; exit 1; }

grep -q '#define NOVA_APP_SECURITY_SOCKET_PATH "/run/nova/app-security.sock"' "$HDR"
grep -q '#define NOVA_APP_SECURITY_PROTOCOL_VERSION 1U' "$HDR"
grep -q 'NOVA_APP_CAP_UI_SURFACE' "$HDR"
grep -q 'NOVA_APP_CAP_INPUT_TEXT' "$HDR"
grep -q 'NOVA_APP_CAP_INPUT_TAB_CAPTURE' "$HDR"
grep -q 'NOVA_APP_CAP_CLIPBOARD_READ' "$HDR"
grep -q 'NOVA_APP_CAP_CLIPBOARD_WRITE' "$HDR"
grep -q 'NOVA_APP_SECURITY_OP_LOOKUP_PID' "$HDR"
grep -q 'NOVA_APP_SECURITY_OP_PRINCIPAL' "$HDR"
grep -q 'declared_capabilities' "$HDR"
grep -q 'effective_capabilities' "$HDR"
grep -q 'app_generation' "$HDR"
grep -q 'policy_generation' "$HDR"
grep -q 'sizeof(struct nova_app_security_request) == 32' "$HDR"
grep -q 'sizeof(struct nova_app_security_reply) == 104' "$HDR"

# Caller-selected app/client identity must not be present in the request.
req_body="$(sed -n '/struct nova_app_security_request {/,/};/p' "$HDR")"
if grep -Eq 'app_id|client_id' <<<"$req_body"; then
  echo 'FAIL: security lookup request carries caller-selected app/client identity' >&2
  exit 1
fi

# The authorization plane must not become a path/command transport.
if grep -Eqi 'char[[:space:]]+(exec|path|argv|environment|command|uri|url)\[' "$HDR"; then
  echo 'FAIL: forbidden executable/path/command field in security ABI' >&2
  exit 1
fi

# Step-1 intentionally does not claim unenforceable FS/network sandbox bits.
if grep -Eqi 'CAP_.*(FILESYSTEM|NETWORK|DEVICE|SYSCALL)' "$HDR"; then
  echo 'FAIL: Step-1 claims capability classes without an enforcement boundary' >&2
  exit 1
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
cat > "$tmp/check.c" <<'C'
#include "nova_app_security_protocol.h"
int main(void) {
    struct nova_app_security_request req = {0};
    struct nova_app_security_reply reply = {0};
    return (sizeof(req) == 32U && sizeof(reply) == 104U &&
            NOVA_APP_CAP_PHASE16_KNOWN_MASK == 31ULL) ? 0 : 1;
}
C
clang -std=c11 -Wall -Wextra -Werror -I"$ROOT/src/runtime" -fsyntax-only "$tmp/check.c"

echo 'Stage-5A Phase-16 authenticated app-principal/capability-policy UAPI contract: PASS'
