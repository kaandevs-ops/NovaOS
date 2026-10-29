#!/usr/bin/env bash
set -uo pipefail
TARGET="${1:-}"
if [[ -z "$TARGET" || ! -d "$TARGET" ]]; then
  echo "usage: $0 <unpacked-candidate-directory>" >&2
  exit 2
fi
failures=0
report() { echo "PACKAGE-HYGIENE: $*" >&2; failures=$((failures+1)); }

if [[ -e "$TARGET/build" ]]; then report "build/ must not be present"; fi
if [[ -e "$TARGET/dist" ]]; then report "dist/ must not be present inside a source candidate"; fi
if [[ -e "$TARGET/.stage4d-backup" ]]; then report ".stage4d-backup/ must not be present inside a source candidate"; fi
if [[ -e "$TARGET/.git" ]]; then report ".git/ metadata must not be present inside a source candidate"; fi
mapfile -t caches < <(find "$TARGET" -type d -name '__pycache__' -o -type f \( -name '*.pyc' -o -name '*.pyo' \) 2>/dev/null)
((${#caches[@]}==0)) || { printf 'cache artifact: %s\n' "${caches[@]}" >&2; report "Python cache artifacts found"; }
mapfile -t keynames < <(find "$TARGET" -type f \( -iname '*.key' -o -iname '*.key.pem' -o -iname '*.p8' -o -iname '*.p12' -o -iname '*.pfx' -o -iname '*private-key*' \) 2>/dev/null)
((${#keynames[@]}==0)) || { printf 'private-key-like filename: %s\n' "${keynames[@]}" >&2; report "private-key-like files found"; }
mapfile -t keymaterial < <(grep -RIlE --exclude='package-hygiene.sh' -- '-----BEGIN ([A-Z0-9 ]+ )?PRIVATE KEY-----' "$TARGET" 2>/dev/null || true)
((${#keymaterial[@]}==0)) || { printf 'private key material: %s\n' "${keymaterial[@]}" >&2; report "PEM private key material found"; }
mapfile -t generated < <(find "$TARGET" -type f \( -name 'OVMF_VARS*.fd' -o -name '*.img' -o -name '*.fat' -o -name '*.raw' -o -name '*.qcow2' -o -name '*.vhdx' \) 2>/dev/null)
((${#generated[@]}==0)) || { printf 'generated build artifact: %s\n' "${generated[@]}" >&2; report "generated disk/firmware artifacts found"; }
if ((failures)); then
  echo "PACKAGE-HYGIENE: FAIL ($failures categories)" >&2
  exit 1
fi
echo "PACKAGE-HYGIENE: PASS"
