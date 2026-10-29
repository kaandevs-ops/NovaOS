#!/usr/bin/env python3
"""Phase-14 Step-2 novad catalog producer model only; not guest runtime."""

MAX=64
OK=0
BAD=1
DENIED=2
HELLO=1
GET=2
HELLO_REPLY=0x100
ENTRY=0x101
END=0x102
ERROR=0x103
VISIBLE=1
LAUNCHABLE=2
STOPPED=1
RUNNING=3


def safe_token(s,limit=32):
    return bool(s) and len(s)<limit and all(c.isalnum() or c in '._-' for c in s)


def safe_name(s,limit=64):
    return bool(s) and len(s)<limit and all(0x20 <= ord(c) <= 0x7e and c not in '/\\' for c in s)


def validate_registry(entries):
    if len(entries)>MAX: return False
    ids=set(); clients=set()
    for e in entries:
        if not safe_token(e['app_id']) or not safe_name(e['display_name']): return False
        if e['client_id']<=0 or e['app_id'] in ids or e['client_id'] in clients: return False
        if any(k in e for k in ('exec','path','argv','environment','command')): return False
        ids.add(e['app_id']); clients.add(e['client_id'])
    return True


def request_valid(q):
    if q.get('magic')!='NCAT' or q.get('version')!=1 or q.get('size')!=32: return False
    if q.get('flags')!=0 or q.get('reserved')!=0: return False
    if q.get('opcode')==HELLO: return q.get('index')==0
    return q.get('opcode')==GET


def reply(entries,q,authorized=True,generation=1):
    base=dict(magic='NCAT',version=1,size=152,request_id=q.get('request_id',0),
              index=q.get('index',0),total_count=len(entries),catalog_generation=generation)
    if not request_valid(q): return dict(base,opcode=ERROR,result=BAD)
    if not authorized: return dict(base,opcode=ERROR,result=DENIED)
    if q['opcode']==HELLO: return dict(base,opcode=HELLO_REPLY,result=OK)
    if q['index']>=len(entries): return dict(base,opcode=END,result=OK)
    e=entries[q['index']]
    flags=VISIBLE | (LAUNCHABLE if e['enabled'] else 0)
    return dict(base,opcode=ENTRY,result=OK,app_id=e['app_id'],client_id=e['client_id'],
                display_name=e['display_name'],state=e['state'],catalog_flags=flags)

apps=[
    dict(app_id='nova.demo.a',client_id=1,display_name='Nova Demo A',state=STOPPED,enabled=True),
    dict(app_id='nova.demo.b',client_id=2,display_name='Nova Demo B',state=RUNNING,enabled=True),
]
assert validate_registry(apps)
hello=dict(magic='NCAT',version=1,size=32,request_id=7,index=0,flags=0,reserved=0,opcode=HELLO)
h=reply(apps,hello,generation=1)
assert (h['opcode'],h['result'],h['total_count'],h['catalog_generation'])==(HELLO_REPLY,OK,2,1)
for i,app in enumerate(apps):
    q=dict(hello,opcode=GET,index=i,request_id=8+i)
    r=reply(apps,q)
    assert r['opcode']==ENTRY and r['app_id']==app['app_id'] and r['client_id']==app['client_id']
    assert r['display_name']==app['display_name'] and r['catalog_flags']==VISIBLE|LAUNCHABLE
end=reply(apps,dict(hello,opcode=GET,index=2))
assert end['opcode']==END and end['result']==OK
assert reply(apps,dict(hello,version=2))['result']==BAD
assert reply(apps,dict(hello,flags=1))['result']==BAD
assert reply(apps,dict(hello,reserved=1))['result']==BAD
assert reply(apps,dict(hello,opcode=999))['result']==BAD
assert reply(apps,hello,authorized=False)['result']==DENIED
assert not validate_registry([dict(apps[0],display_name='bad/path')])
assert not validate_registry([apps[0],dict(apps[1],client_id=1)])
assert not validate_registry([dict(apps[0],exec='/System/Apps/x')])
assert not validate_registry([dict(apps[0],display_name='X'*64)])
assert not validate_registry([dict(app_id=f'nova.app.{i}',client_id=i+1,display_name='App',state=STOPPED,enabled=True) for i in range(65)])

# State/catalog projection intentionally contains no executable metadata.
projection=[{k:e[k] for k in ('app_id','client_id','display_name','state')} for e in apps]
assert all('exec' not in e and 'path' not in e for e in projection)

print('Stage-5A Phase-14 novad catalog producer/authorization/enumeration model unit: PASS')
