#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/src/runtime/nova_tpm2.c"
python3 - "$SRC" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
assert '#define TPM2_CC_PCR_READ        0x0000017eUL' in s
start=s.index('static int read_pcr_sha256_direct')
end=s.index('int nova_tpm2_read_pcr_sha256_sysfs_only', start)
b=s[start:end]
required=(
    'be32_put(cmd+o,TPM2_CC_PCR_READ)',
    'be32_put(cmd+o,1)',
    'be16_put(cmd+o,TPM2_ALG_SHA256)',
    'cmd[o++]=3',
    '1U<<(pcr%8U)',
    'requested_seen',
    'digests<1UL',
    'digests>8UL',
    'dsz!=32U',
    'digest_to_hex(rsp+at,32U,hex_out)',
)
for needle in required:
    assert needle in b, needle
assert b.index('TPM2_CC_PCR_READ') < b.index('tpm_xfer(cmd,o,rsp,sizeof(rsp),&rn)')
# Golden request vectors for the exact one-bank/3-byte selection layout used by NovaOS.
def req(pcr):
    sel=[0,0,0]; sel[pcr//8]=1<<(pcr%8)
    body=(1).to_bytes(4,'big')+(0x000b).to_bytes(2,'big')+bytes([3,*sel])
    size=10+len(body)
    return (0x8001).to_bytes(2,'big')+size.to_bytes(4,'big')+(0x17e).to_bytes(4,'big')+body
assert req(7).hex()=='8001000000140000017e00000001000b03800000'
assert req(14).hex()=='8001000000140000017e00000001000b03004000'
print('Stage-4C direct TPM2_PCR_Read packet/parser source policy + golden vectors passed.')
PY
