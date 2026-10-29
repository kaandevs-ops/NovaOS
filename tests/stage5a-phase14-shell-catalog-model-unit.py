#!/usr/bin/env python3
"""Phase-14 shell catalog cache/dispatch model; not QEMU runtime acceptance."""

MAX=64
VISIBLE=1
LAUNCHABLE=2
VALID_FLAGS=VISIBLE|LAUNCHABLE


def token(value,cap):
    return bool(value) and len(value)<cap and all(c.isalnum() or c in '._-' for c in value)


def display(value,cap):
    return bool(value) and len(value)<cap and all(0x20 <= ord(c) <= 0x7e and c not in '/\\' for c in value)


def enumerate_catalog(hello,entries,end):
    count,generation=hello
    if count>MAX:
        raise ValueError('oversized')
    cache=[]
    seen_ids=set(); seen_clients=set()
    for index,e in enumerate(entries):
        if index>=count:
            raise ValueError('too-many')
        if e['index']!=index or e['total']!=count or e['generation']!=generation:
            raise ValueError('snapshot-drift')
        if not e['client'] or not token(e['app_id'],32) or not display(e['name'],64):
            raise ValueError('invalid-metadata')
        if e['flags'] & ~VALID_FLAGS:
            raise ValueError('unknown-flags')
        if e['app_id'] in seen_ids or e['client'] in seen_clients:
            raise ValueError('duplicate')
        seen_ids.add(e['app_id']); seen_clients.add(e['client']); cache.append(dict(e))
    if len(cache)!=count:
        raise ValueError('short-enumeration')
    if end != (count,generation):
        raise ValueError('bad-end')
    return cache


def app_for_client(cache,client):
    for e in cache:
        if e['client']==client:
            return e
    return None

entries=[
    dict(index=0,total=2,generation=1,client=1,app_id='nova.demo.a',name='Nova Demo A',flags=VISIBLE|LAUNCHABLE,state=1),
    dict(index=1,total=2,generation=1,client=2,app_id='nova.demo.b',name='Nova Demo B',flags=VISIBLE|LAUNCHABLE,state=1),
]
cache=enumerate_catalog((2,1),entries,(2,1))
assert app_for_client(cache,1)['app_id']=='nova.demo.a'
assert app_for_client(cache,2)['app_id']=='nova.demo.b'
assert app_for_client(cache,99) is None
# Lifecycle identity comes from app_id, never display_name.
assert app_for_client(cache,1)['app_id'] != app_for_client(cache,1)['name']

bad=[dict(entries[0]),dict(entries[1])]
bad[1]['client']=1
try: enumerate_catalog((2,1),bad,(2,1)); raise AssertionError('duplicate client accepted')
except ValueError: pass
bad=[dict(entries[0]),dict(entries[1])]
bad[1]['app_id']='nova.demo.a'
try: enumerate_catalog((2,1),bad,(2,1)); raise AssertionError('duplicate app accepted')
except ValueError: pass
bad=[dict(entries[0]),dict(entries[1])]
bad[1]['name']='/System/Apps/evil'
try: enumerate_catalog((2,1),bad,(2,1)); raise AssertionError('path-like display accepted')
except ValueError: pass
bad=[dict(entries[0]),dict(entries[1])]
bad[1]['generation']=2
try: enumerate_catalog((2,1),bad,(2,1)); raise AssertionError('generation drift accepted')
except ValueError: pass
try: enumerate_catalog((65,1),[],(65,1)); raise AssertionError('oversized catalog accepted')
except ValueError: pass

print('Stage-5A Phase-14 shell bounded catalog cache/client-binding model unit: PASS')
