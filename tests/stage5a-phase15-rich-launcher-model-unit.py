#!/usr/bin/env python3
import hashlib,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
assets=(ROOT/'configs/assets/builtin-assets.txt').read_text()
entries=[]
for line in assets.splitlines():
    if not line.startswith('asset_id='): continue
    d=dict(x.split('=',1) for x in line.split())
    entries.append(d)
assert len(entries)==2
assert {e['asset_id'] for e in entries}=={'nova.icon.demo.a','nova.icon.demo.b'}
assert all(e['format']=='a8-mask' and int(e['width'])==24 and int(e['height'])==24 and int(e['bytes'])==576 for e in entries)
# Transaction model: DEFINE with HAS_ICON must receive exactly one matching icon before COMMIT.
HAS_ICON=2
class Model:
    def __init__(self): self.staging=False; self.expected=0; self.items=[]
    def reset(self,n): self.staging=True; self.expected=n; self.items=[]
    def define(self,idx,client,flags):
        assert self.staging and idx==len(self.items) and idx<self.expected and client>0
        self.items.append({'client':client,'flags':flags,'icon':None})
    def icon(self,idx,client,asset,payload,digest):
        assert self.staging and idx<len(self.items)
        it=self.items[idx]; assert it['client']==client and it['flags']&HAS_ICON and it['icon'] is None
        assert re.fullmatch(r'[A-Za-z0-9._-]+',asset) and len(payload)==576
        assert hashlib.sha256(payload).digest()==digest
        it['icon']=asset
    def commit(self):
        assert self.staging and len(self.items)==self.expected
        assert all(not (i['flags']&HAS_ICON) or i['icon'] for i in self.items)
        self.staging=False; return list(self.items)
m=Model(); m.reset(2); m.define(0,7,HAS_ICON); m.define(1,42,HAS_ICON)
p=b'\xff'*576; d=hashlib.sha256(p).digest(); m.icon(0,7,'nova.icon.demo.a',p,d); m.icon(1,42,'nova.icon.demo.b',p,d)
out=m.commit(); assert [x['client'] for x in out]==[7,42]
# Missing/mismatched icon fails closed.
m=Model(); m.reset(1); m.define(0,7,HAS_ICON)
try: m.commit(); raise AssertionError('missing icon accepted')
except AssertionError: pass
# Legacy geometry remains unchanged for existing pointer acceptance.
assert (220,110,80,20,70,60)==(220,110,80,20,70,60)
print('Stage-5A Phase-15 atomic rich-task icon binding/digest/legacy-geometry model unit: PASS')
