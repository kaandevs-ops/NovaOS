#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Stage-5 development identity moves forward while the accepted Stage-4D
# regression wrapper intentionally keeps rebuilding the released 0.0.11 image.
grep -Fq 'VERSION="${NOVA_VERSION:-0.0.12-dev}"' "$ROOT/scripts/build-userland.sh"
grep -Fq 'VERSION="${NOVA_VERSION:-0.0.12-dev}"' "$ROOT/scripts/build-disk.sh"
grep -Fq 'VERSION="${NOVA_VERSION:-0.0.12-dev}"' "$ROOT/scripts/verity/prepare-recovery-artifact.sh"
grep -Fq '#define NOVA_VERSION "0.0.12-dev"' "$ROOT/src/init/main.c"
grep -Fq 'NOVA_VERSION=0.0.11 NOVA_RELEASE_NEXT_KEY_ID=' "$ROOT/scripts/windows/final-stage4d-check.sh"

# Storage format identity is independent from product version.
grep -Fq 'novaos-stage4c.img' "$ROOT/scripts/build-disk.sh"
grep -Fq 'nova-stage4c-gpt-v1' "$ROOT/scripts/build-disk.sh"

echo 'NovaOS v0.0.12-dev Stage-5 development identity + frozen v0.0.11 Stage-4D regression identity: PASS'
