#!/usr/bin/env python3
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
TOKEN=re.compile(r'^[A-Za-z0-9._-]+$')
CATEGORIES={
    'unspecified':0,'system':1,'productivity':2,'development':3,
    'media':4,'communication':5,'game':6,'utility':7,
}
HAS_DESCRIPTION=1<<0
HAS_ICON=1<<1

def parse_lines(text):
    out={}
    for raw in text.splitlines():
        line=raw.strip()
        if not line or line.startswith('#'): continue
        k,sep,v=line.partition('=')
        assert sep and k not in out
        out[k]=v
    return out

def description_valid(v):
    return bool(v) and len(v.encode())<128 and all(0x20<=ord(c)<=0x7e for c in v)

def icon_valid(v):
    return bool(v) and len(v.encode())<48 and bool(TOKEN.fullmatch(v)) and not any(c in v for c in '/\\:')

def validate_manifest(d):
    is_app=d.get('kind','service')=='application'
    metadata_keys={'category','description','icon_asset_id'} & d.keys()
    if not is_app:
        return not metadata_keys
    if d.get('category') not in CATEGORIES:
        return False
    desc=d.get('description','')
    icon=d.get('icon_asset_id','')
    if desc and not description_valid(desc): return False
    if icon and not icon_valid(icon): return False
    return True

def project(d,index,total,generation):
    assert validate_manifest(d)
    desc=d.get('description','')
    icon=d.get('icon_asset_id','')
    flags=(HAS_DESCRIPTION if desc else 0)|(HAS_ICON if icon else 0)
    return dict(index=index,total_count=total,catalog_generation=generation,
                app_id=d['app_id'],client_id=int(d['client_id']),
                category=CATEGORIES[d['category']],metadata_flags=flags,
                description=desc,icon_asset_id=icon)

apps=[]
for name in ('demo-app.service','demo-app-b.service'):
    d=parse_lines((ROOT/'configs/services'/name).read_text())
    assert validate_manifest(d),name
    apps.append(d)

assert [a['category'] for a in apps]==['development','development']
assert apps[0]['icon_asset_id']=='nova.icon.demo.a'
assert apps[1]['icon_asset_id']=='nova.icon.demo.b'

meta=[project(a,i,2,1) for i,a in enumerate(apps)]
for i,(a,r) in enumerate(zip(apps,meta)):
    assert r['index']==i and r['total_count']==2 and r['catalog_generation']==1
    assert r['app_id']==a['app_id'] and r['client_id']==int(a['client_id'])
    assert r['category']==3 and r['metadata_flags']==HAS_DESCRIPTION|HAS_ICON
    assert r['description']==a['description'] and r['icon_asset_id']==a['icon_asset_id']

# Invalid category/path/URI/control/overflow metadata must fail closed.
for category in ('admin','root','Development',''):
    d=dict(apps[0],category=category)
    assert not validate_manifest(d)
for icon in ('/System/Icons/a.png','../a.png','file:a.png','https://x/y','nova/icon/a','nova\\icon\\a','x'*48):
    d=dict(apps[0],icon_asset_id=icon)
    assert not validate_manifest(d),icon
for desc in ('line1\nline2','x'*128):
    d=dict(apps[0],description=desc)
    assert not validate_manifest(d)

# Optional description/icon are allowed and flags must exactly mirror presence.
d=dict(apps[0]); d.pop('description'); d.pop('icon_asset_id')
r=project(d,0,1,5)
assert r['metadata_flags']==0 and r['description']=='' and r['icon_asset_id']==''

# Rich metadata is application-only.
svc={'name':'heartbeat','exec':'/System/Core/nova-heartbeat','kind':'service','category':'utility'}
assert not validate_manifest(svc)

echo='Stage-5A Phase-15 trusted manifest metadata/category/pathless-icon producer model unit: PASS'
print(echo)
