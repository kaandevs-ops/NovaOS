#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; S="$ROOT/scripts/windows/setup-wsl.sh"
for pkg in ovmf sbsigntool dosfstools mtools python3-virt-firmware; do grep -Fq "$pkg" "$S"; done
grep -Fq 'final-tpm-check.sh' "$S"
echo "Stage-4C WSL UEFI/Secure Boot toolchain policy passed."
