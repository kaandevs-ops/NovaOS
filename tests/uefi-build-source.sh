#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
E="$ROOT/scripts/uefi/build-esp.sh"; O="$ROOT/scripts/uefi/prepare-ovmf-vars.sh"; D="$ROOT/scripts/build-disk.sh"
for needle in 'sbsign --key' 'sbverify --cert' 'mkfs.vfat -F 32 -n NOVA_ESP' '::/EFI/BOOT/$EFI_NAME' 'esp-${MODE}.fat' 'initramfs_binding=builtin-in-efi-kernel'; do grep -Fq -- "$needle" "$E"; done
for needle in '--set-pk' '--add-kek' '--add-db' '--secure-boot' 'OVMF_CODE_4M.secboot.fd' 'OVMF_VARS_4M.fd'; do grep -Fq -- "$needle" "$O"; done
grep -Fq 'esp-signed.fat' "$D"
grep -Fq "str(s/'payloads/esp.fat')" "$D"
! grep -Fq 'ESP-RESERVED' "$D"

python3 - "$D" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
for name in ("NOVA_ESP","NOVA_SYSTEM_A","NOVA_SYSTEM_B","NOVA_RECOVERY","NOVA_DATA"):
    marker=f'{name}\",\"type\"'
    assert s.count(marker)==1, f"partition uniqueness failed for {name}: {s.count(marker)}"
assert '"size_bytes":64*M' in s, 'Stage-4B ESP must be 64 MiB'
PY
echo "Stage-4B GPT partition uniqueness + 64 MiB ESP source policy passed."
echo "Stage-4B signed FAT ESP + OVMF enrollment source policy passed."
