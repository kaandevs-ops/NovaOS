#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/services/assets.c"
[[ -f "$SRC" ]]
grep -q 'NOVA_ASSET_CLIENTS 4U' "$SRC"
grep -q 'nova.icon.demo.a' "$SRC"
grep -q 'nova.icon.demo.b' "$SRC"
grep -q 'NOVA_ASSET_FLAG_IMMUTABLE|NOVA_ASSET_FLAG_BUILTIN' "$SRC"
grep -q 'NOVA_MSG_TRUNC|NOVA_MSG_CTRUNC' "$SRC"
grep -q 'r->length<=NOVA_ASSET_CHUNK_BYTES' "$SRC"
grep -q 'expanded_byte(asset,request.offset+i)' "$SRC"
grep -q 'nova_sha256_hex' "$SRC"
grep -q 'build_native "$ROOT/src/services/assets.c" "$ROOTFS/System/Core/nova-assets"' "$ROOT/scripts/build-userland.sh"
grep -q 'nova-assets' "$ROOT/scripts/build-initramfs.sh"
grep -q '/System/Services/assets.service' "$ROOT/configs/services/services.list"
grep -q 'exec=/System/Core/nova-assets' "$ROOT/configs/services/assets.service"
# Asset IDs in application metadata must be registered in the trusted asset inventory.
while IFS='=' read -r key value; do
  [[ "$key" == icon_asset_id ]] || continue
  grep -q "^asset_id=$value " "$ROOT/configs/assets/builtin-assets.txt"
done < <(grep -h '^icon_asset_id=' "$ROOT"/configs/services/demo-app*.service)
echo 'Stage-5A Phase-15 supervised immutable asset-registry/build/rootfs wiring source contract: PASS'
