#!/usr/bin/env python3
"""Phase-14 catalog contract model only; this is not a runtime/novad test."""

MAX_ENTRIES = 64
VISIBLE = 1 << 0
LAUNCHABLE = 1 << 1
STOPPED = 1
RUNNING = 3


def valid_token(value: str, limit: int = 32) -> bool:
    if not value or len(value.encode('utf-8')) >= limit:
        return False
    return all(ch.isalnum() or ch in '._-' for ch in value)


def validate_registry(entries):
    if len(entries) > MAX_ENTRIES:
        return False
    ids=set(); clients=set()
    for e in entries:
        if not valid_token(e['app_id']):
            return False
        if not isinstance(e['client_id'], int) or e['client_id'] <= 0:
            return False
        if e['app_id'] in ids or e['client_id'] in clients:
            return False
        if not e['display_name'] or len(e['display_name'].encode('utf-8')) >= 64:
            return False
        if any(k in e for k in ('exec','path','argv','environment','command')):
            return False
        if e['state'] not in (1,2,3,4,5,6):
            return False
        ids.add(e['app_id']); clients.add(e['client_id'])
    return True


def catalog_get(entries,index,generation=1):
    assert validate_registry(entries)
    assert 0 <= index <= 0xffffffff
    if index >= len(entries):
        return {
            'opcode':'END','index':index,'total_count':len(entries),
            'catalog_generation':generation,'result':'OK'
        }
    e=entries[index]
    return {
        'opcode':'ENTRY','index':index,'total_count':len(entries),
        'catalog_generation':generation,'result':'OK',
        'app_id':e['app_id'],'client_id':e['client_id'],
        'display_name':e['display_name'],'state':e['state'],
        'catalog_flags':e['catalog_flags'],
    }


apps=[
    dict(app_id='nova.demo.a',client_id=1,display_name='Nova Demo A',state=STOPPED,catalog_flags=VISIBLE|LAUNCHABLE),
    dict(app_id='nova.demo.b',client_id=2,display_name='Nova Demo B',state=RUNNING,catalog_flags=VISIBLE|LAUNCHABLE),
]
assert validate_registry(apps)
a=catalog_get(apps,0,7)
b=catalog_get(apps,1,7)
end=catalog_get(apps,2,7)
assert (a['app_id'],a['client_id'],a['state']) == ('nova.demo.a',1,STOPPED)
assert (b['app_id'],b['client_id'],b['state']) == ('nova.demo.b',2,RUNNING)
assert end == {'opcode':'END','index':2,'total_count':2,'catalog_generation':7,'result':'OK'}
assert a['catalog_generation']==b['catalog_generation']==7

bad=list(apps)+[dict(apps[0],display_name='Duplicate')]
assert not validate_registry(bad)
bad=[apps[0],dict(apps[1],client_id=1)]
assert not validate_registry(bad)
bad=[dict(apps[0],exec='/System/Apps/evil')]
assert not validate_registry(bad)
bad=[dict(apps[0],display_name='x'*64)]
assert not validate_registry(bad)
assert not validate_registry([dict(app_id=f'nova.test.{i}',client_id=i+1,display_name='X',state=STOPPED,catalog_flags=VISIBLE) for i in range(MAX_ENTRIES+1)])

print('Stage-5A Phase-14 bounded catalog enumeration/uniqueness/no-exec model unit: PASS')
