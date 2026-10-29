#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/src/runtime/nova_surface_protocol.h"
grep -Eq '#define NOVA_SURFACE_PROTOCOL_VERSION [3456]U' "$P"
grep -Fq '#define NOVA_SURFACE_MAX_CLIENT_BUFFERS 8U' "$P"
grep -Fq '#define NOVA_SURFACE_MAX_BUFFER_BYTES (8U * 1024U * 1024U)' "$P"
grep -Fq '#define NOVA_PIXEL_FORMAT_XRGB8888 0x34325258U' "$P"
for op in CREATE_BUFFER DESTROY_BUFFER CREATE_SURFACE SET_POSITION ATTACH_BUFFER DAMAGE COMMIT HEARTBEAT; do grep -Fq "NOVA_SURFACE_OP_${op}" "$P"; done
for field in pixel_format stride buffer_bytes damage_x damage_y damage_width damage_height; do grep -Fq "$field" "$P"; done
grep -Eq 'sizeof\(struct nova_surface_message\) == (88|144|152|216)' "$P"
grep -Eq 'sizeof\(struct nova_surface_event\) == (44|64|136)' "$P"
echo 'Stage-5A Phase-6 client-buffer UAPI compatibility contract: PASS'
