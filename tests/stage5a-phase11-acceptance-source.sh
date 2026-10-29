#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; F="$ROOT/scripts/windows/stage5a-phase11-check.py"
for marker in \
  "client_connection_count')==2" "client1_surface_count')==2" "client2_surface_count')==2" \
  "client1_buffer_count')==2" "client2_buffer_count')==2" \
  "surface100_buffer_id')==1" "surface200_buffer_id')==1" \
  "clipboard_owner_client')==1" "cross-client clipboard paste" \
  "service kill demo-app" "App-B remains focusable" "text_value')=='NOVAX'" \
  'wait_client_redraw_presented' "client{client_id}_commits" \
  'stage5a-phase11-multi-client.ppm' 'stage5a-phase11-cross-client-clipboard.ppm' 'stage5a-phase11-crash-isolation.ppm'; do
  grep -Fq "$marker" "$F" || { echo "missing Phase-11 acceptance marker: $marker" >&2; exit 1; }
done
echo 'Stage-5A Phase-11 OVMF/TPM/multi-client/cross-clipboard/crash-isolation screenshot acceptance contract: PASS'
