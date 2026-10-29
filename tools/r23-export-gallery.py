#!/usr/bin/env python3
"""Lossless native captures and equal-state comparisons; no synthesized desktop content."""
from pathlib import Path
import subprocess,sys,json,struct,zlib,hashlib
r=Path(__file__).resolve().parents[1];e=r/'evidence/r23';out=e/'R23_BEFORE_AFTER';out.mkdir(exist_ok=True)
def read(p):
 with p.open('rb') as f:
  assert f.readline().strip()==b'P6';w,h=map(int,f.readline().split());assert f.readline().strip()==b'255';d=f.read();assert len(d)==w*h*3;return w,h,d
def png(p,w,h,data):
 def chunk(k,d):return struct.pack('!I',len(d))+k+d+struct.pack('!I',zlib.crc32(k+d)&0xffffffff)
 desc=b'Native NovaOS R22 host framebuffer; NOT VirtualBox' if 'before' in p.parts else b'Native NovaOS R23 host framebuffer; NOT VirtualBox'
 if p.parent.name=='R23_BEFORE_AFTER':desc=b'Native host comparison: R22 left, R23 right; NOT VirtualBox'
 raw=b''.join(b'\0'+data[y*w*3:(y+1)*w*3] for y in range(h))
 p.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('!2I5B',w,h,8,2,0,0,0))+chunk(b'tEXt',b'Description\0'+desc)+chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))
checks=[]
for directory in [e/'before/gallery',e/'gallery']:
 for p in sorted(directory.glob('*.ppm')):
  w,h,d=read(p);png(p.with_suffix('.png'),w,h,d)
for p in sorted((e/'before/gallery').glob('*.ppm')):
 if p.stem.endswith('settings-before'):continue
 q=e/'gallery'/p.name;w,h,a=read(p);ww,hh,b=read(q);assert (w,h)==(ww,hh)
 sa=json.loads(p.with_name(p.stem+'-state.json').read_text());sb=json.loads(q.with_name(q.stem+'-state.json').read_text())
 # Registry slot order follows process connection timing; compare semantic surfaces by stable ID.
 sa['surfaces']=sorted(sa['surfaces'],key=lambda x:x['id']);sb['surfaces']=sorted(sb['surfaces'],key=lambda x:x['id']);assert sa==sb,(p.name,sa,sb)
 joined=b''.join(a[y*w*3:(y+1)*w*3]+b[y*w*3:(y+1)*w*3] for y in range(h));target=out/(p.stem+'-R22-left-R23-right.png');png(target,w*2,h,joined)
 checks.append({'state':p.stem,'width':w,'height':h,'geometry_focus_state_equal':True,'r22_reference_sha256':hashlib.sha256(a).hexdigest(),'r23_sha256':hashlib.sha256(b).hexdigest(),'comparison':str(target.relative_to(r))})
assert len(checks)==15
# A/B/C use the same native renderer and same committed surface geometry.
faces=[read(e/'gallery'/('1600x900-'+name+'.ppm')) for name in ['nova-settings-light','nova-settings-graphite','nova-settings-adaptive']]
assert all((w,h)==(1600,900) for w,h,d in faces)
joined=b''.join(b''.join(d[y*1600*3:(y+1)*1600*3] for w,h,d in faces) for y in range(900));png(e/'R23_GLOBAL_THEME_SETTINGS_HOST_NOT_VM.png',4800,900,joined)
for p in e.glob('*.ppm'):
 w,h,d=read(p);png(p.with_suffix('.png'),w,h,d)
(e/'visual-matrix.json').write_text(json.dumps({'status':'PASS','scope':'equal native capture geometry and buffer integrity; aesthetic review is separate','pairs':checks,'r23_captures':len(list((e/'gallery').glob('*.ppm')))},indent=2));print('15 equal-state comparisons exported')

a=e/'before/gallery/1600x900-settings-before.ppm';b=e/'gallery/1600x900-nova-settings-adaptive.ppm'
if a.exists() and b.exists():
 w,h,pa=read(a);ww,hh,pb=read(b);assert (w,h)==(ww,hh)
 data=b''.join(pa[y*w*3:(y+1)*w*3]+pb[y*w*3:(y+1)*w*3] for y in range(h));png(out/'1600x900-settings-evolution-R22-left-R23-right.png',w*2,h,data)

a=e/'before/gallery/1600x900-settings-before.ppm';b=e/'gallery/1600x900-system-overview-adaptive.ppm'
w,h,pa=read(a);ww,hh,pb=read(b);assert (w,h)==(ww,hh)
data=b''.join(pa[y*w*3:(y+1)*w*3]+pb[y*w*3:(y+1)*w*3] for y in range(h));png(out/'1600x900-overview-feature-evolution-DIFFERENT-PAGES.png',w*2,h,data)
