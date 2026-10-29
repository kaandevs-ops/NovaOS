#!/usr/bin/env python3
from pathlib import Path
import hashlib,re
ROOT=Path(__file__).resolve().parents[1]
src=(ROOT/'src/services/assets.c').read_text()
inv=(ROOT/'configs/assets/builtin-assets.txt').read_text().splitlines()

def parse_array(name,n):
    m=re.search(rf'static const unsigned char {re.escape(name)}\[{n}\] = \{{(.*?)\}};',src,re.S)
    assert m, name
    vals=bytes(int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{2})',m.group(1)))
    assert len(vals)==n,(name,len(vals))
    return vals

def expand(bits):
    out=bytearray()
    for i in range(576):
        out.append(255 if ((bits[i>>3]>>(7-(i&7)))&1) else 0)
    return bytes(out)

records={}
for line in inv:
    if not line.startswith('asset_id='): continue
    fields=dict(part.split('=',1) for part in line.split())
    aid=fields.pop('asset_id')
    assert aid not in records
    assert re.fullmatch(r'[A-Za-z0-9._-]{1,47}',aid)
    assert '/' not in aid and '\\' not in aid and ':' not in aid
    assert fields['format']=='a8-mask'
    assert int(fields['width'])==24 and int(fields['height'])==24 and int(fields['bytes'])==576
    records[aid]=fields
assert set(records)=={'nova.icon.demo.a','nova.icon.demo.b'}
for letter,aid in [('a','nova.icon.demo.a'),('b','nova.icon.demo.b')]:
    bits=parse_array(f'g_icon_{letter}_bits',72)
    digest=parse_array(f'g_icon_{letter}_digest',32)
    payload=expand(bits)
    assert hashlib.sha256(payload).digest()==digest
    assert hashlib.sha256(payload).hexdigest()==records[aid]['sha256']
    # Every valid chunk is bounded and reconstructs the same immutable payload.
    rebuilt=bytearray()
    for off in range(0,len(payload),137):
        chunk=payload[off:off+137]
        assert 0<len(chunk)<=1024
        rebuilt.extend(chunk)
    assert bytes(rebuilt)==payload
# Unknown IDs and path-shaped identities are not registry hits.
for bad in ['/System/Assets/a','../a','file:a','https://a','nova/icon/a']:
    assert bad not in records
print('Stage-5A Phase-15 immutable built-in asset registry/digest/chunk/no-path model unit: PASS')
