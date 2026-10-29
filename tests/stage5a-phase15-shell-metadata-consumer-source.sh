#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
S="$ROOT/src/services/shell.c"
H="$ROOT/src/runtime/nova_app_metadata_protocol.h"

for marker in \
  '#include "nova_app_metadata_protocol.h"' \
  'unsigned int category;' \
  'unsigned int metadata_flags;' \
  'char description[NOVA_APP_DESCRIPTION_BYTES];' \
  'char icon_asset_id[NOVA_APP_ICON_ASSET_ID_BYTES];' \
  'catalog_description_valid' \
  'catalog_icon_asset_id_valid' \
  'catalog_metadata_fields_valid' \
  'catalog_metadata_reply_header_valid' \
  'catalog_metadata_request' \
  'NOVA_APP_CATALOG_OP_GET_METADATA' \
  'NOVA_APP_CATALOG_OP_METADATA' \
  'NOVA_MSG_TRUNC' \
  'NOVA_MSG_CTRUNC' \
  'app_metadata_extension=phase15-v1' \
  'app_metadata_requests=' \
  'app_metadata_replies=' \
  'app_metadata_rejected=' \
  '_category=' \
  '_metadata_flags=' \
  '_description=' \
  '_icon_asset_id=' \
  'app_metadata_entry index=' \
  'app_metadata=CONNECTED extension=phase15-v1'; do
  grep -Fq "$marker" "$S" || { echo "missing Phase-15 shell metadata marker: $marker" >&2; exit 1; }
done

python3 - "$S" <<'PY'
import re,sys
s=open(sys.argv[1],encoding='utf-8').read()

def fn(name, next_name=None):
    start=s.index('static int '+name) if ('static int '+name) in s else s.index('static void '+name)
    if next_name:
        end=s.index(next_name,start)
    else:
        m=re.search(r'\nstatic (?:int|void) [A-Za-z0-9_]+\(',s[start+1:])
        end=start+1+m.start() if m else len(s)
    return s[start:end]

meta_req=fn('catalog_metadata_request','static int catalog_enumerate')
for token in ['NOVA_APP_CATALOG_OP_GET_METADATA','nova_recvmsg','NOVA_MSG_TRUNC','NOVA_MSG_CTRUNC',
              'catalog_metadata_reply_header_valid','g_catalog.metadata_requests++','g_catalog.metadata_replies++']:
    assert token in meta_req, token

meta_valid=fn('catalog_metadata_fields_valid','static struct shell_catalog_entry')
for token in ['NOVA_APP_CATEGORY_UTILITY','NOVA_APP_METADATA_FLAG_HAS_DESCRIPTION','NOVA_APP_METADATA_FLAG_HAS_ICON',
              'catalog_description_valid','catalog_icon_asset_id_valid']:
    assert token in meta_valid, token

enum=s[s.index('static int catalog_enumerate'):s.index('static int lifecycle_launch')]
for token in ['catalog_metadata_request(fd, index','metadata.index != index','metadata.total_count != expected_count',
              'metadata.catalog_generation != expected_generation','metadata.client_id != entry->client_id',
              'nova_streq(metadata.app_id, entry->app_id)','catalog_metadata_fields_valid(&metadata)',
              'entry->category = metadata.category','entry->metadata_flags = metadata.metadata_flags',
              'entry->description','entry->icon_asset_id']:
    assert token in enum, token
# Base identity must be installed before metadata binding is checked.
assert enum.index('copy_bounded_string(entry->app_id') < enum.index('catalog_metadata_request(fd, index')
# Count commits only after metadata validation/copy succeeds.
assert enum.index('catalog_metadata_fields_valid(&metadata)') < enum.index('g_catalog.count++')

launch=s[s.index('static int lifecycle_launch'):s.index('static int lifecycle_stop')]
stop=s[s.index('static int lifecycle_stop'):s.index('static int respond_to_task_event')]
for block in (launch,stop):
    assert 'entry->app_id' in block
    assert 'entry->description' not in block
    assert 'entry->icon_asset_id' not in block
    assert 'entry->category' not in block
    assert 'entry->metadata_flags' not in block

# Rich metadata is presentation-only and no resource/executable path is introduced.
for forbidden in ['/System/Apps/','file://','http://','https://','exec=']:
    assert forbidden not in s

connect=s[s.index('static int connect_once'):s.index('int nova_main')]
assert 'catalog_enumerate((int)catalog_fd' in connect
assert 'app_metadata=CONNECTED extension=phase15-v1' in connect
assert 'g_catalog.reconnects++' in connect

state=s[s.index('static void write_state'):s.index('static int send_scene_message')]
# Preserve Phase-14 schema compatibility; Phase-15 fields are an additive extension.
assert 'schema=3' in state
for key in ['app_metadata_extension=phase15-v1','app_metadata_requests=','app_metadata_replies=',
            'app_metadata_rejected=','_category=','_metadata_flags=','_description=','_icon_asset_id=']:
    assert key in state, key
PY

CC="${CC:-clang}"
"$CC" -std=c11 -Wall -Wextra -Werror -fsyntax-only \
  -I"$ROOT/src/runtime" "$S"

echo 'Stage-5A Phase-15 nova-shell snapshot-bound rich-metadata cache/reconnect source contract: PASS'
