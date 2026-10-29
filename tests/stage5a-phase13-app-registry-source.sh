#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
N="$ROOT/src/init/main.c"
A="$ROOT/configs/services/demo-app.service"
B="$ROOT/configs/services/demo-app-b.service"

for marker in \
  '#include "nova_app_protocol.h"' \
  'char app_id[NOVA_APP_ID_BYTES]' \
  'unsigned int client_id' \
  'int autostart' \
  'int is_application' \
  'safe_metadata_token(value,sizeof(svc->app_id))' \
  'parse_unsigned_canonical(value,&v)' \
  'service_registry_conflicts' \
  'find_application_by_id' \
  'find_application_by_client_id' \
  'application_registry_consistent' \
  'invalid or conflicting service manifest skipped' \
  'g_services[i].enabled && g_services[i].autostart'
do
    grep -Fq "$marker" "$N" || {
        echo "missing Phase-13 app-registry novad marker: $marker" >&2
        exit 1
    }
done

for spec in \
  "$A|nova.demo.a|1" \
  "$B|nova.demo.b|2"
do
    IFS='|' read -r manifest app_id client_id <<<"$spec"
    grep -qx 'kind=application' "$manifest"
    grep -qx "app_id=$app_id" "$manifest"
    grep -qx "client_id=$client_id" "$manifest"
    grep -qx 'autostart=no' "$manifest"
    grep -qx 'restart=always' "$manifest"
    grep -qx 'enabled=yes' "$manifest"
done

CC="${CC:-clang}"

"$CC" \
  -std=c11 \
  -Wall \
  -Wextra \
  -Werror \
  -ffreestanding \
  -fno-builtin \
  -fsyntax-only \
  -I"$ROOT/src/runtime" \
  -DNOVA_VERSION=\"0.0.12-dev\" \
  -DNOVA_BUILD_ARCH=\"x86_64\" \
  -DNOVA_ROOT_CERT_SHA256=\"UNPINNED\" \
  -DNOVA_TRUST_PROFILE=\"development\" \
  -DNOVA_TRUST_KEY_ID=\"nova-development-root-2026\" \
  -DNOVA_UEFI_CERT_SHA256=\"UNPINNED\" \
  -DNOVA_RELEASE_KEY_ID=\"nova-development-release-2026-a\" \
  -DNOVA_TRUST_EPOCH=1 \
  "$N"

echo 'Stage-5A Phase-13 novad application-registry/non-autostart source contract: PASS'
