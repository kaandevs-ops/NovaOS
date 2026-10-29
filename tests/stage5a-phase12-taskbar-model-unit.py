#!/usr/bin/env python3
# Deterministic model of the bounded Phase-12 panel hit regions.  This catches
# overlap/gap regressions without requiring a QEMU pointer device.
ACTIVATE, CLOSE, LAUNCH = 1, 2, 3
def hit(x,y):
    if y < 6 or y >= 32: return None
    for i in range(2):
        x0=152+i*112
        if x0<=x<x0+80:return (i+1,ACTIVATE)
        if x0+80<=x<x0+100:return (i+1,CLOSE)
        lx=152+2*112+10+i*70
        if lx<=x<lx+60:return (i+1,LAUNCH)
    return None
vectors={(182,20):(1,ACTIVATE),(242,20):(1,CLOSE),(294,20):(2,ACTIVATE),(354,20):(2,CLOSE),(416,20):(1,LAUNCH),(486,20):(2,LAUNCH),(258,20):None,(182,32):None}
for p,want in vectors.items():
    assert hit(*p)==want,(p,hit(*p),want)
print('Stage-5A Phase-12 bounded updated native panel geometry model: PASS')
