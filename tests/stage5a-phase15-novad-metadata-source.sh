#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
M="$ROOT/src/init/main.c"
H="$ROOT/src/runtime/nova_app_metadata_protocol.h"
A="$ROOT/configs/services/demo-app.service"
B="$ROOT/configs/services/demo-app-b.service"

for marker in \
  '#include "nova_app_metadata_protocol.h"' \
  'char description[NOVA_APP_DESCRIPTION_BYTES];' \
  'char icon_asset_id[NOVA_APP_ICON_ASSET_ID_BYTES];' \
  'unsigned int category;' \
  'static int safe_app_description(' \
  'static int application_category_from_name(' \
  'static unsigned int application_metadata_flags(' \
  'NOVA_APP_CATALOG_OP_GET_METADATA' \
  'static void fill_app_metadata_reply(' \
  'static int send_app_metadata_reply(' \
  'metadata_extension=phase15-v1' \
  '_metadata_flags=' \
  '_description=' \
  '_icon_asset_id='; do
  grep -Fq "$marker" "$M" || { echo "missing Phase-15 novad metadata marker: $marker" >&2; exit 1; }
done

for manifest in "$A" "$B"; do
  grep -qx 'kind=application' "$manifest"
  grep -qx 'category=development' "$manifest"
  grep -Eq '^description=[ -~]+$' "$manifest"
  grep -Eq '^icon_asset_id=[A-Za-z0-9._-]+$' "$manifest"
  ! grep -Eq '^icon_asset_id=.*[/\\:]' "$manifest"
done
grep -qx 'description=NovaOS development application for UI and lifecycle validation' "$A"
grep -qx 'icon_asset_id=nova.icon.demo.a' "$A"
grep -qx 'description=Second isolated NovaOS development application' "$B"
grep -qx 'icon_asset_id=nova.icon.demo.b' "$B"

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

parse=body('parse_manifest')
for marker in ('metadata_seen','"category"','application_category_from_name',
               '"description"','safe_app_description',
               '"icon_asset_id"','safe_metadata_token(value,sizeof(svc->icon_asset_id))'):
    assert marker in parse,marker
assert '!(metadata_seen&1U)' in parse
assert 'metadata_seen!=0U' in parse

valid=body('app_catalog_request_valid')
assert 'NOVA_APP_CATALOG_OP_GET_METADATA' in valid

fill=body('fill_app_metadata_reply')
for marker in ('NOVA_APP_CATALOG_PROTOCOL_MAGIC','NOVA_APP_CATALOG_PROTOCOL_VERSION',
               'NOVA_APP_CATALOG_OP_METADATA','application_catalog_count()',
               'g_app_catalog_generation','svc->app_id','svc->client_id','svc->category',
               'application_metadata_flags(svc)','svc->description','svc->icon_asset_id'):
    assert marker in fill,marker
for forbidden in ('svc->exec','argv','environment','command','/System/Apps'):
    assert forbidden not in fill,forbidden

proc=body('process_app_catalog_packet')
assert 'request.opcode==NOVA_APP_CATALOG_OP_GET_METADATA' in proc
assert 'fill_app_metadata_reply(&metadata,&request,svc)' in proc
assert 'send_app_metadata_reply(fd,&metadata)' in proc

state=body('write_app_catalog_state')
for marker in ('"schema=2"','"metadata_extension=phase15-v1"','svc->category',
               'application_metadata_flags(svc)','svc->description','svc->icon_asset_id'):
    assert marker in state,marker
assert 'svc->exec' not in state

consistent=body('application_registry_consistent')
for marker in ('NOVA_APP_CATEGORY_UTILITY','safe_app_description','safe_metadata_token(svc->icon_asset_id'):
    assert marker in consistent,marker
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

echo 'Stage-5A Phase-15 novad trusted-manifest rich-metadata producer source contract: PASS'
