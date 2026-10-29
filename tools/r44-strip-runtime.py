"""Remove unused symbols from OWN runtime ELFs; preserve loadable bytes/ABI.
Never strips catalog payloads, libraries, third-party archives or old trees.
"""
from pathlib import Path
import struct,subprocess,sys,json,hashlib
r=Path(__file__).resolve().parents[1];a=sys.argv[1];assert a in ('x86_64','arm64');root=r/f'build/{a}/rootfs';assert root.is_dir()
strip='strip' if a=='x86_64' else 'aarch64-linux-gnu-strip'
def loads(data):
 assert data[:6]==b'\x7fELF\x02\x01';h=struct.unpack_from('<16sHHIQQQIHHHHHH',data);entry=h[4];off=h[5];ents=h[9];count=h[10];out=[]
 for i in range(count):
  p=struct.unpack_from('<IIQQQQQQ',data,off+i*ents)
  if p[0]==1:
   content=bytearray(data[p[2]:p[2]+p[5]])
   # ELF section-header metadata is not a loaded program behavior.
   if p[2]==0:
    content[40:48]=b'\0'*8;content[58:64]=b'\0'*6
   out.append((p,bytes(content)))
 return entry,out
files=list((root/'System/Core').glob('nova-*'))+list((root/'System/Terminal/bin').glob('nova-*'));result={}
for f in files:
 if not f.is_file() or f.is_symlink():continue
 before=f.read_bytes()
 if before[:4]!=b'\x7fELF':continue
 expected=loads(before);subprocess.run([strip,'--strip-unneeded',str(f)],check=True);after=f.read_bytes();assert loads(after)==expected,'loadable code/data or ABI changed: '+str(f)
 result[str(f.relative_to(root))]={'before':len(before),'after':len(after),'load_segments_identical':True,'sha256':hashlib.sha256(after).hexdigest()}
e=r/'evidence/r44';e.mkdir(parents=True,exist_ok=True);(e/f'strip-runtime-{a}.json').write_text(json.dumps(result,indent=2));print('R44_RUNTIME_STRIP_LOAD_SEGMENTS_PASS',a,sum(v['before']-v['after'] for v in result.values()),'bytes saved')
