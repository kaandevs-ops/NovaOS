#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
S="$ROOT/scripts/build-update-bundle.sh"
for needle in \
  'update.manifest' 'update.sig' 'system.ext4' \
  'openssl dgst -sha256 -sign' \
  'openssl dgst -sha256 -verify' \
  'payload_sha256=' 'payload_size=' \
  'NOVA_UPDATE_SYSTEM'; do grep -Fq "$needle" "$S"; done
# Private signing material must be sourced from build/dev-keys or explicit env, never copied to bundle.
! grep -Eq 'cp .*\.key|cp .*SIGNING_KEY.*update-bundle' "$S"
echo "Stage-3B update bundle build policy test passed."
