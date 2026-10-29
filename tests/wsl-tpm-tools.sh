#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; S="$ROOT/scripts/windows/setup-wsl.sh"
for pkg in swtpm swtpm-tools tpm2-tools libtss2-tcti-swtpm0t64; do grep -Fq "$pkg" "$S"; done
grep -Fq 'final-tpm-check.sh' "$S"
echo 'Stage-4C WSL swtpm/TPM2 toolchain policy passed.'
