#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
C="$ROOT/src/services/compositor.c"

for marker in \
  'static int connection_has_capability(' \
  'static int surface_has_capability(' \
  'static int requested_input_flags_authorized(' \
  'NOVA_APP_CAP_INPUT_TEXT' \
  'NOVA_APP_CAP_INPUT_TAB_CAPTURE' \
  'NOVA_APP_CAP_CLIPBOARD_READ' \
  'NOVA_APP_CAP_CLIPBOARD_WRITE' \
  'reason=capability_denied capability=' \
  'capability_denials=' \
  'input_flag_denials=' \
  'input_text_denials=' \
  'tab_capture_denials=' \
  'clipboard_read_denials=' \
  'clipboard_write_denials='; do
  grep -Fq "$marker" "$C" || { echo "missing Phase-16 capability-enforcement marker: $marker" >&2; exit 1; }
done

python3 - "$C" <<'PY'
import re,sys
text=open(sys.argv[1],encoding='utf-8').read()

def body(name):
    m=re.search(r'static\s+[^\n]+\s+'+re.escape(name)+r'\s*\([^)]*\)\s*\{',text)
    assert m,name
    i=m.end(); depth=1
    while i<len(text) and depth:
        if text[i]=='{': depth+=1
        elif text[i]=='}': depth-=1
        i+=1
    assert depth==0,name
    return text[m.start():i]

flags=body('requested_input_flags_authorized')
assert 'NOVA_SURFACE_INPUT_CAPTURE_TAB' in flags and 'NOVA_APP_CAP_INPUT_TAB_CAPTURE' in flags
assert 'NOVA_SURFACE_INPUT_TEXT_ASCII' in flags and 'NOVA_APP_CAP_INPUT_TEXT' in flags
assert 'NOVA_SURFACE_INPUT_CLIPBOARD' in flags
assert 'NOVA_APP_CAP_CLIPBOARD_READ' in flags and 'NOVA_APP_CAP_CLIPBOARD_WRITE' in flags

setcb=body('set_client_clipboard')
assert 'NOVA_APP_CAP_CLIPBOARD_WRITE' in setcb
assert setcb.index('NOVA_APP_CAP_CLIPBOARD_WRITE') < setcb.index('client_clipboard_allowed')

getcb=body('get_client_clipboard')
assert 'NOVA_APP_CAP_CLIPBOARD_READ' in getcb
assert getcb.index('NOVA_APP_CAP_CLIPBOARD_READ') < getcb.index('client_clipboard_allowed')

sendcb=body('send_clipboard_event')
assert 'NOVA_APP_CAP_CLIPBOARD_READ' in sendcb

sendev=body('send_client_event')
assert 'type==NOVA_SURFACE_EVENT_KEY' in sendev
assert 'code==NOVA_KEY_TAB' in sendev
assert 'NOVA_APP_CAP_INPUT_TAB_CAPTURE' in sendev
assert 'NOVA_APP_CAP_INPUT_TEXT' in sendev

consume=body('consume_client_message')
assert 'requested_input_flags_authorized(conn,m.input_flags)' in consume
# Capability denial handles an operation but must not tear down the authenticated
# connection as a side effect. consume() returns handled=1 for denied flag/clipboard ops.
assert 'if (m.opcode==NOVA_SURFACE_OP_SET_CLIPBOARD) return set_client_clipboard(surface,&m);' in consume
assert 'if (m.opcode==NOVA_SURFACE_OP_GET_CLIPBOARD) return get_client_clipboard(surface);' in consume

# Frozen Phase-9 TAB-routing marker remains additive and now has a capability gate.
assert 'ev->code==NOVA_KEY_TAB && focused_client && (focused_client->input_flags&NOVA_SURFACE_INPUT_CAPTURE_TAB)' in text
assert 'surface_has_capability(focused_client,NOVA_APP_CAP_INPUT_TAB_CAPTURE)' in text
PY

CC="${CC:-clang}"
for ARCH in x86_64 aarch64; do
  TARGET="x86_64-linux-gnu"; MACRO=x86_64
  [[ "$ARCH" == aarch64 ]] && { TARGET="aarch64-linux-gnu"; MACRO=aarch64; }
  "$CC" --target="$TARGET" -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only \
    -I"$ROOT/src/runtime" \
    -DNOVA_VERSION=\"0.0.12-dev\" -DNOVA_BUILD_ARCH=\"$MACRO\" \
    "$C"
done

echo 'Stage-5A Phase-16 authenticated compositor input/clipboard capability enforcement source contract: PASS'
