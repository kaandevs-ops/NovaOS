#!/usr/bin/env python3
"""Model the Phase-15 shell rich-metadata snapshot/cache contract."""

MAX=64
CATEGORIES=set(range(0,8))
HAS_DESCRIPTION=1<<0
HAS_ICON=1<<1
ALLOWED_FLAGS=HAS_DESCRIPTION|HAS_ICON

def token(value, allow_empty=False, limit=48):
    if not isinstance(value,str) or len(value)>=limit: return False
    if not value: return allow_empty
    return all(c.isascii() and (c.isalnum() or c in '._-') for c in value)

def description(value):
    return isinstance(value,str) and len(value)<128 and all(0x20 <= ord(c) <= 0x7e for c in value)

def validate_meta(meta,base,count,generation,index):
    if meta.get('index')!=index or meta.get('total_count')!=count or meta.get('generation')!=generation: return False
    if meta.get('app_id')!=base['app_id'] or meta.get('client_id')!=base['client_id']: return False
    if meta.get('category') not in CATEGORIES: return False
    flags=meta.get('flags',0)
    if flags & ~ALLOWED_FLAGS: return False
    desc=meta.get('description','')
    icon=meta.get('icon','')
    if not description(desc) or not token(icon,allow_empty=True): return False
    if bool(flags & HAS_DESCRIPTION) != bool(desc): return False
    if bool(flags & HAS_ICON) != bool(icon): return False
    return True

def build_cache(base_entries,metadata,count,generation):
    if count>MAX or len(base_entries)!=count or len(metadata)!=count: raise ValueError('count')
    seen_ids=set(); seen_clients=set(); result=[]
    for i,(base,meta) in enumerate(zip(base_entries,metadata)):
        if base['app_id'] in seen_ids or base['client_id'] in seen_clients: raise ValueError('duplicate')
        if not token(base['app_id'],limit=32) or not base['client_id']: raise ValueError('identity')
        if not validate_meta(meta,base,count,generation,i): raise ValueError('metadata-binding')
        seen_ids.add(base['app_id']); seen_clients.add(base['client_id'])
        result.append({**base,'category':meta['category'],'description':meta['description'],'icon':meta['icon'],'metadata_flags':meta['flags']})
    return result

base=[
 {'app_id':'nova.demo.a','client_id':1,'display':'Nova Demo A'},
 {'app_id':'nova.demo.b','client_id':2,'display':'Nova Demo B'},
]
meta=[
 {'index':0,'total_count':2,'generation':7,'app_id':'nova.demo.a','client_id':1,'category':3,'flags':3,
  'description':'NovaOS development application for UI and lifecycle validation','icon':'nova.icon.demo.a'},
 {'index':1,'total_count':2,'generation':7,'app_id':'nova.demo.b','client_id':2,'category':3,'flags':3,
  'description':'Second isolated NovaOS development application','icon':'nova.icon.demo.b'},
]
cache=build_cache(base,meta,2,7)
assert cache[0]['icon']=='nova.icon.demo.a' and cache[1]['category']==3

# Reconnect/reconstruction yields a fresh equivalent cache and does not depend on old presentation state.
cache2=build_cache([dict(x) for x in base],[dict(x) for x in meta],2,7)
assert cache2==cache and cache2 is not cache

bad=[]
for mutate in [
 lambda m: m.update(generation=8),
 lambda m: m.update(total_count=3),
 lambda m: m.update(index=1),
 lambda m: m.update(app_id='nova.demo.b'),
 lambda m: m.update(client_id=99),
 lambda m: m.update(category=99),
 lambda m: m.update(flags=8),
 lambda m: m.update(icon='/System/Icons/demo.png'),
 lambda m: m.update(icon='file:demo'),
 lambda m: m.update(flags=HAS_DESCRIPTION,icon='nova.icon.demo.a'),
 lambda m: m.update(flags=HAS_ICON,description='unexpected'),
]:
    mm=[dict(x) for x in meta]
    mutate(mm[0])
    try: build_cache(base,mm,2,7)
    except ValueError: bad.append(True)
    else: bad.append(False)
assert all(bad)

# Presentation metadata cannot replace application identity in lifecycle lookup.
by_client={x['client_id']:x['app_id'] for x in cache}
assert by_client[1]=='nova.demo.a'
cache[0]['description']='Totally different presentation text'
cache[0]['icon']='nova.icon.changed'
assert by_client[1]=='nova.demo.a'

print('Stage-5A Phase-15 shell rich-metadata snapshot/cache/no-authority model unit: PASS')
