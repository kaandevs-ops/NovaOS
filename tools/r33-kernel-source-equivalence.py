from pathlib import Path
import hashlib,json,sys
r=Path(__file__).resolve().parents[1];local,ref=map(lambda x:Path(x).resolve(),sys.argv[1:3]);phase=sys.argv[3]
assert local==r/'build/src/linux-6.18.54' and local!=ref and phase in ('before','after')
def digest(root):
 h=hashlib.sha256();count=0
 for p in sorted(root.rglob('*')):
  rel=str(p.relative_to(root)).encode()
  if p.is_symlink():h.update(b'L'+rel+b'\0'+str(p.readlink()).encode()+b'\0');count+=1
  elif p.is_file():
   h.update(b'F'+rel+b'\0');h.update(hashlib.sha256(p.read_bytes()).digest());count+=1
 return h.hexdigest(),count
left,n=digest(local);right,m=digest(ref);assert left==right and n==m,'kernel source reference differs from isolated copy'
e=r/'evidence/r33';e.mkdir(parents=True,exist_ok=True);record={'status':'PASS','local':str(local),'readonly_reference':str(ref),'sha256':left,'records':n,'phase':phase}
if phase=='after':assert json.loads((e/'kernel-source-before.json').read_text())['sha256']==left,'kernel input tree changed during build'
(e/('kernel-source-'+phase+'.json')).write_text(json.dumps(record,indent=2));print('KERNEL_SOURCE_EQUIVALENCE_PASS',phase,n,left)
