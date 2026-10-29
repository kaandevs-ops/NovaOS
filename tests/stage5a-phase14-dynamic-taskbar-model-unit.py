#!/usr/bin/env python3
MAX=64
LAUNCHABLE=1
ACTIVATE,CLOSE,LAUNCH=1,2,3

class Model:
    def __init__(self):
        self.active=[]; self.gen=0; self.staging=None
    def reset(self,count,gen):
        if not (0 <= count <= MAX): return False
        self.staging={'expected':count,'gen':gen,'items':[]}; return True
    def define(self,index,total,client,flags,name,gen):
        s=self.staging
        valid=bool(s and total==s['expected'] and gen==s['gen'] and index==len(s['items']) and index<total
                   and client>0 and not (flags & ~LAUNCHABLE) and name and '/' not in name and '\\' not in name
                   and not any(x['client']==client for x in s['items']))
        if not valid:
            self.staging=None; return False
        s['items'].append({'client':client,'flags':flags,'name':name}); return True
    def commit(self,index,total,gen):
        s=self.staging
        valid=bool(s and index==s['expected'] and total==s['expected'] and gen==s['gen'] and len(s['items'])==s['expected'])
        if not valid:
            self.staging=None; return False
        self.active=list(s['items']); self.gen=gen; self.staging=None; return True

def hit(tasks,x,y):
    if y<6 or y>=32: return None
    task_base,task_stride,task_width,close_width=152,112,80,20
    launch_stride,launch_width=70,60
    launch_base=task_base+len(tasks)*task_stride+10
    for i,t in enumerate(tasks):
        tx=task_base+i*task_stride
        if tx<=x<tx+task_width: return (t['client'],ACTIVATE)
        if tx+task_width<=x<tx+task_width+close_width: return (t['client'],CLOSE)
        lx=launch_base+i*launch_stride
        if t['flags']&LAUNCHABLE and lx<=x<lx+launch_width: return (t['client'],LAUNCH)
    return None

m=Model()
assert m.reset(2,7)
assert m.define(0,2,1,LAUNCHABLE,'Nova Demo A',7)
assert not m.define(1,2,1,LAUNCHABLE,'Duplicate',7)
assert m.reset(2,7)
assert m.define(0,2,1,LAUNCHABLE,'Nova Demo A',7)
assert m.define(1,2,2,LAUNCHABLE,'Nova Demo B',7)
assert not m.commit(1,2,7)
assert m.staging is None
assert m.reset(2,7)
assert m.define(0,2,1,LAUNCHABLE,'Nova Demo A',7)
assert m.define(1,2,2,LAUNCHABLE,'Nova Demo B',7)
assert m.commit(2,2,7)
assert [x['client'] for x in m.active]==[1,2]
vectors={(182,20):(1,ACTIVATE),(242,20):(1,CLOSE),(294,20):(2,ACTIVATE),(354,20):(2,CLOSE),(416,20):(1,LAUNCH),(486,20):(2,LAUNCH),(258,20):None,(182,32):None}
for p,want in vectors.items():
    got=hit(m.active,*p)
    if got!=want: raise SystemExit(f'dynamic taskbar compatibility mismatch {p}: got={got} want={want}')
assert m.reset(3,8)
assert m.define(0,3,7,LAUNCHABLE,'Editor',8)
assert m.define(1,3,42,0,'Settings',8)
assert m.define(2,3,99,LAUNCHABLE,'Terminal',8)
assert m.commit(3,3,8)
assert hit(m.active,182,20)==(7,ACTIVATE)
assert hit(m.active,294,20)==(42,ACTIVATE)
assert hit(m.active,406,20)==(99,ACTIVATE)
launch_base=152+3*112+10
assert hit(m.active,launch_base+10,20)==(7,LAUNCH)
assert hit(m.active,launch_base+70+10,20) is None
assert hit(m.active,launch_base+140+10,20)==(99,LAUNCH)
print('Stage-5A Phase-14 atomic dynamic task-model/geometry/arbitrary-client model unit: PASS')
