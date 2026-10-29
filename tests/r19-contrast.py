#!/usr/bin/env python3
from pathlib import Path
import subprocess,json
r=Path(__file__).resolve().parents[1];out=r/'build/x86_64/r17-native';source=r/'tests/r19-palettes.c'
subprocess.run(['clang','-std=c11','-I',str(r/'src/runtime'),str(source),'-o',str(out/'r19-palette-reader')],check=True)
def lum(c):
 v=[((c>>s)&255)/255 for s in (16,8,0)];v=[x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4 for x in v];return sum(a*b for a,b in zip(v,(.2126,.7152,.0722)))
checks={}
for line in subprocess.check_output([str(out/'r19-palette-reader')],text=True).splitlines():
 t,paper,sidebar,ink,muted,selection=map(int,line.split())
 for name,fg,bg in [('body',ink,paper),('muted',muted,paper),('sidebar',muted,sidebar),('selection',ink,selection)]:
  a,b=sorted((lum(fg),lum(bg)));ratio=(b+.05)/(a+.05);checks[f'{t}-{name}']={'ratio':round(ratio,3),'PASS':ratio>=4.5}
result={'scope':'actual native C palettes, 3 themes x 4 opaque text/background pairs; not entire product accessibility certification','checks':checks,'status':'PASS' if all(v['PASS'] for v in checks.values()) else 'FAIL'}
(r/'evidence/r21/contrast.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2));assert result['status']=='PASS'
