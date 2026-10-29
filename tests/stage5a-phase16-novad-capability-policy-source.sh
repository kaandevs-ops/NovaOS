#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
M="$ROOT/src/init/main.c"
A="$ROOT/configs/services/demo-app.service"
B="$ROOT/configs/services/demo-app-b.service"

for marker in \
  '#include "nova_app_security_protocol.h"' \
  '#define NOVA_APP_SECURITY_STATE_FILE "/run/nova/app-security.state"' \
  'unsigned long long declared_capabilities;' \
  'unsigned long long effective_capabilities;' \
  'static unsigned long long g_app_policy_generation = 0ULL;' \
  'static int application_capability_token(' \
  'static int parse_application_capabilities(' \
  'static unsigned long long application_policy_effective_capabilities(' \
  'static int application_capability_policy_valid(' \
  'static void write_app_security_state(' \
  'policy_kind=manifest-capability-policy-v1' \
  'policy_authorized=' \
  '_declared_mask=' \
  '_effective_mask=' \
  '_declared=' \
  '_effective='; do
  grep -Fq "$marker" "$M" || { echo "missing Phase-16 novad capability-policy marker: $marker" >&2; exit 1; }
done

canonical='capabilities=ui.surface,input.text,input.tab-capture,clipboard.read,clipboard.write'
for manifest in "$A" "$B"; do
  grep -qx 'kind=application' "$manifest"
  grep -qx "$canonical" "$manifest"
  [[ "$(grep -c '^capabilities=' "$manifest")" -eq 1 ]]
done

python3 - "$M" <<'PY'
import re,sys
m=open(sys.argv[1],encoding='utf-8').read()

def body(name):
    mm=re.search(r'static\s+[^\n]+\s+'+re.escape(name)+r'\s*\([^)]*\)\s*\{',m)
    assert mm,name
    i=mm.end(); depth=1
    while i<len(m) and depth:
        if m[i]=='{': depth+=1
        elif m[i]=='}': depth-=1
        i+=1
    assert depth==0,name
    return m[mm.start():i]

parse_caps=body('parse_application_capabilities')
for marker in ('"none"','application_capability_token','rank<=previous_rank',
               'mask&bit','NOVA_APP_CAP_PHASE16_KNOWN_MASK'):
    assert marker in parse_caps,marker

parse=body('parse_manifest')
for marker in ('capabilities_seen','"capabilities"','parse_application_capabilities',
               '!capabilities_seen','application_policy_effective_capabilities',
               'application_capability_policy_valid'):
    assert marker in parse,marker
# Application manifests require an explicit declaration; services may not carry it.
assert '|| !capabilities_seen' in parse
assert '|| capabilities_seen' in parse

valid=body('application_capability_policy_valid')
assert 'NOVA_APP_CAP_PHASE16_KNOWN_MASK' in valid
assert 'effective_capabilities&~svc->declared_capabilities' in valid

consistent=body('application_registry_consistent')
assert 'application_capability_policy_valid(svc)' in consistent

state=body('write_app_security_state')
for marker in ('"schema=1"','"policy_kind=manifest-capability-policy-v1"','"policy_authorized="',
               'NOVA_APP_SECURITY_PROTOCOL_VERSION','NOVA_APP_CAP_PHASE16_KNOWN_MASK',
               'g_app_policy_generation','svc->declared_capabilities',
               'svc->effective_capabilities','write_capability_list'):
    assert marker in state,marker
for forbidden in ('svc->exec','argv','environment','command'):
    assert forbidden not in state,forbidden

load=body('load_services')
assert 'g_app_policy_generation=1ULL' in load
assert 'write_app_security_state()' in load
PY

CC="${CC:-clang}"
"$CC" \
  -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fsyntax-only \
  -I"$ROOT/src/runtime" \
  -DNOVA_VERSION=\"0.0.12-dev\" \
  -DNOVA_BUILD_ARCH=\"x86_64\" \
  -DNOVA_ROOT_CERT_SHA256=\"UNPINNED\" \
  -DNOVA_TRUST_PROFILE=\"development\" \
  -DNOVA_TRUST_KEY_ID=\"nova-development-root-2026\" \
  -DNOVA_UEFI_CERT_SHA256=\"UNPINNED\" \
  -DNOVA_RELEASE_KEY_ID=\"nova-development-release-2026-a\" \
  -DNOVA_TRUST_EPOCH=1 \
  "$M"

echo 'Stage-5A Phase-16 novad strict manifest capability-policy registry/state source contract: PASS'
