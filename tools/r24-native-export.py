from pathlib import Path
import json,struct,zlib,hashlib,os
r=Path(__file__).resolve().parents[1];e=Path(os.environ.get('NOVA_R24_EVIDENCE_DIR',str(r/'evidence/r24/evolution-20261021')));before=r/'evidence/r24/gallery';gallery=e/'gallery';comparisons=e/'comparisons';comparisons.mkdir(exist_ok=True)
def read(p):
 with p.open('rb') as f:
  assert f.readline().strip()==b'P6';w,h=map(int,f.readline().split());assert f.readline().strip()==b'255';data=f.read();assert len(data)==w*h*3;return w,h,data
def png(p,w,h,data):
 def chunk(t,d):return struct.pack('!I',len(d))+t+d+struct.pack('!I',zlib.crc32(t+d)&0xffffffff)
 raw=b''.join(b'\0'+data[y*w*3:(y+1)*w*3] for y in range(h));p.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('!2I5B',w,h,8,2,0,0,0))+chunk(b'tEXt',b'Description\0Actual native NovaOS CPU renderer; HOST NOT VIRTUALBOX')+chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))
results=[];intentional={'1600x900-graphics-diagnostics-host-no-samples','1280x800-graphics-diagnostics-narrow'}
for old in sorted(before.glob('*.ppm')):
 new=gallery/old.name;assert new.exists(),old.name;w,h,a=read(old);ww,hh,b=read(new);assert(w,h)==(ww,hh)
 sa=json.loads(old.with_name(old.stem+'-state.json').read_text());sb=json.loads(new.with_name(new.stem+'-state.json').read_text());sa['surfaces'].sort(key=lambda x:x['id']);sb['surfaces'].sort(key=lambda x:x['id']);assert sa==sb,old.name
 masks=[]
 if 'system-overview' in old.stem:
  left=w-560-20+154;y=64;wide=560-176
  masks=[(left,y+72,wide,28),(left+wide//2,y+128-22,wide//2,28)]
 if old.stem in intentional:
  status='INTENTIONAL_UI_CHANGE';joined=b''.join(a[y*w*3:(y+1)*w*3]+b[y*w*3:(y+1)*w*3] for y in range(h));png(comparisons/(old.stem+'-before-left-after-right-HOST.png'),w*2,h,joined)
 elif masks:
  aa=bytearray(a);bb=bytearray(b)
  for x,y,mw,mh in masks:
   for yy in range(y,y+mh):start=(yy*w+x)*3;aa[start:start+mw*3]=b'\0'*(mw*3);bb[start:start+mw*3]=b'\0'*(mw*3)
  assert aa==bb,('unexpected pixels outside build/uptime fields',old.name);status='PIXEL_IDENTICAL_OUTSIDE_BUILD_ID_UPTIME'
 else:assert a==b,('unexpected visual change',old.name);status='PIXEL_IDENTICAL'
 results.append({'case':old.stem,'status':status,'semantic_state_equal':True,'dynamic_field_rects':masks,'before_sha256':hashlib.sha256(a).hexdigest(),'after_sha256':hashlib.sha256(b).hexdigest()})
for p in [*gallery.glob('*.ppm'),*e.glob('*HOST-NOT-VM.ppm')]:w,h,d=read(p);png(p.with_suffix('.png'),w,h,d)
(e/'native-visual-regression.json').write_text(json.dumps({'scope':'actual CPU renderer; scene state equal; known build ID and uptime rectangles excluded ONLY in Overview; two explicitly redesigned Graphics screens reviewed separately','cases':results,'new_cases':sorted(p.stem for p in gallery.glob('*.ppm') if not(before/p.name).exists())},indent=2));print('PASS',len(results),'baseline cases; original captures unmodified')
