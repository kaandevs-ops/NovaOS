#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/scripts/windows/stage5a-phase7-check.py"; F="$ROOT/scripts/windows/final-stage5a-phase7-check.sh"
for marker in \
  "protocol=4 lifecycle=toplevel-configure-ack buffer_transport=memfd-scm-rights" \
  "mouse_button 1" "mouse_button 0" "surface100_z')>state_int(s,'surface101_z')" \
  "surface100_x')==580" "surface100_y')==320" "surface100_width')==400" "surface100_height')==280" \
  "client_configures_sent')>cfg_before" "client_configure_acks')>ack_before" "surface100_awaiting_configure')==0" \
  "post-resize old-buffer cleanup" "client_buffer_count')==2" \
  "surface101 close request delivery" "explicit surface101 destruction" "surface101 buffer destruction" \
  "surface101_closed')==1" "client_explicit_surface_destroys')>destroy_before" "service kill demo-app" \
  'stage5a-phase7-toplevels.ppm' 'stage5a-phase7-raised.ppm' 'stage5a-phase7-moved.ppm' 'stage5a-phase7-resized.ppm' 'stage5a-phase7-closed.ppm' \
  "expect_pixel(shots['resized'],950,570,(24,72,120),'resized client-owned buffer new area')" \
  "expect_pixel(shots['closed'],1040,520,(19,26,36),'closed toplevel removed from scene')"; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-7 acceptance marker: $marker" >&2; exit 1; }
done
grep -Fq 'run-stage5a-phase7-package-suite.sh' "$F"
grep -Fq 'stage5a-phase7-check.py' "$F"
echo 'Stage-5A Phase-7 OVMF/TPM/toplevel-move-resize-close screenshot acceptance contract: PASS'
