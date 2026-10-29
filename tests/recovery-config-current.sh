#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CFG="$ROOT/configs/recovery/recovery.conf"
grep -Fqx 'signed_recovery=dm-verity-verified' "$CFG"
! grep -Fq 'not-yet-implemented' "$CFG"
echo 'Stage-4C Recovery configuration truthfulness regression passed.'
