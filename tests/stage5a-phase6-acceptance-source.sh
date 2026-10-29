#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; P="$ROOT/scripts/windows/stage5a-phase6-check.py"; F="$ROOT/scripts/windows/final-stage5a-phase6-check.sh"
for marker in \
  "protocol=3 buffer_transport=memfd-scm-rights" "client_buffer_count')==2" "client_buffers_created')>=2" \
  "buffer1_bytes')==320*220*4" "buffer2_bytes')==300*200*4" \
  "expect_pixel(shots['a'],740,430,(24,72,120),'surface100 client blue background')" \
  "expect_pixel(shots['a'],770,440,(255,204,0),'surface100 client yellow tile')" \
  "expect_pixel(shots['a'],920,540,(0,204,204),'surface101 client cyan tile')" \
  "state_int(s,'surface100_x')==700" "state_int(s,'client_damage_messages')>damage_before" \
  "expect_pixel(shots['update'],770,440,(24,72,120),'old client tile location repainted by app')" \
  "expect_pixel(shots['update'],830,440,(255,204,0),'new client tile location from shared buffer')" \
  "client_buffer_count')==0" "client_buffers_removed')>=buffers_removed_before+2" \
  'stage5a-phase6-client-buffers.ppm' 'stage5a-phase6-buffer-focus.ppm' 'stage5a-phase6-buffer-updated.ppm' 'stage5a-phase6-buffer-cleanup.ppm'; do
  grep -Fq "$marker" "$P" || { echo "missing Phase-6 acceptance marker: $marker" >&2; exit 1; }
done
grep -Fq 'run-stage5a-phase6-package-suite.sh' "$F"
grep -Fq 'stage5a-phase6-check.py' "$F"
grep -Fq 'System/Apps/nova-demo-app' "$F"
echo 'Stage-5A Phase-6 OVMF/TPM/memfd-pixel-buffer/damage/crash-cleanup screenshot acceptance contract: PASS'
