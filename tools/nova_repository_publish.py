#!/usr/bin/env python3
"""Offline signed repository publication. No network, install or private key output.
Input JSON: array of {id,version,arch,sha256,payload}. Payload paths are relative
regular files below --payload-root. Output must not already exist.
"""
import argparse,hashlib,json,os,re,stat,time
from pathlib import Path
from urllib.parse import urlsplit
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding,PublicFormat

def publish(manifest,root,keyfile,dest,base,serial,expires):
 u=urlsplit(base)
 if u.scheme!='https' or not u.hostname or u.username or u.password or u.query or u.fragment or u.port not in (None,443):raise ValueError('HTTPS origin/path required, port 443 only')
 if not re.fullmatch(r'https://[A-Za-z0-9.-]+(?:/[A-Za-z0-9_./-]*)?',base) or '..' in u.path.split('/'):raise ValueError('noncanonical URL')
 if not 1<=serial<2**63 or not int(time.time())<expires<2**63:raise ValueError('invalid serial/expiry')
 root=Path(root).resolve(strict=True);dest=Path(dest)
 if dest.exists():raise ValueError('output already exists; publication never overwrites')
 fd=os.open(keyfile,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  st=os.fstat(fd)
  if not stat.S_ISREG(st.st_mode) or st.st_uid!=os.geteuid() or st.st_mode&0o077 or st.st_size!=32:raise ValueError('private key must be caller-owned private 32-byte regular file')
  key=Ed25519PrivateKey.from_private_bytes(os.read(fd,33))
 finally:os.close(fd)
 entries=json.loads(Path(manifest).read_text());seen=set();payloads=[];lines=[]
 if not isinstance(entries,list) or not 1<=len(entries)<=64:raise ValueError('entry count')
 for e in entries:
  if not isinstance(e,dict) or set(e)!={'id','version','arch','sha256','payload'}:raise ValueError('entry schema')
  if not all(isinstance(v,str) for v in e.values()):raise ValueError('entry types')
  if not re.fullmatch('[a-z0-9][a-z0-9.-]{0,47}',e['id']) or e['id'] in seen:raise ValueError('invalid or duplicate ID')
  seen.add(e['id'])
  if not re.fullmatch('[A-Za-z0-9][A-Za-z0-9._+-]{0,31}',e['version']) or e['arch'] not in ('x86_64','arm64'):raise ValueError('version/arch')
  rel=Path(e['payload'])
  if rel.is_absolute() or '..' in rel.parts:raise ValueError('payload traversal')
  path=root
  for part in rel.parts:
   path=path/part
   if path.is_symlink():raise ValueError('payload symlink')
  fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
  try:
   st=os.fstat(fd)
   if not stat.S_ISREG(st.st_mode) or not 0<st.st_size<=8388608:raise ValueError('payload type/size')
   with os.fdopen(os.dup(fd),'rb') as f:data=f.read(8388609)
  finally:os.close(fd)
  digest=hashlib.sha256(data).hexdigest()
  if len(data)!=st.st_size or digest!=e['sha256']:raise ValueError('payload hash mismatch')
  name=e['id']+'-'+e['version']+'-'+e['arch']
  url=base.rstrip('/')+'/'+name
  if len(url)>512:raise ValueError('URL too long')
  lines.append(f"{e['id']}\t{e['version']}\t{e['arch']}\t{len(data)}\t{digest}\t{url}\n")
  payloads.append((name,data))
 body=(f'NOVA-REPO-1\t{serial}\t{expires}\n'+''.join(lines)).encode('ascii')
 if len(body)>65536:raise ValueError('index too large')
 # All validation completes before exclusive creation. Incomplete publication
 # remains visibly marked, never merged or silently replaced.
 dest.mkdir(mode=0o700)
 (dest/'INCOMPLETE').write_text('Do not serve until this marker is removed.\n')
 for name,data in payloads:
  with (dest/name).open('xb') as f:f.write(data);f.flush();os.fsync(f.fileno())
 for name,data in [('repository.nri',key.sign(body)+body),('repo.pub',key.public_key().public_bytes(Encoding.Raw,PublicFormat.Raw))]:
  with (dest/name).open('xb') as f:f.write(data);f.flush();os.fsync(f.fileno())
 (dest/'INCOMPLETE').unlink()
 d=os.open(dest,os.O_RDONLY|os.O_DIRECTORY)
 try:os.fsync(d)
 finally:os.close(d)
 return {'entries':len(entries),'serial':serial,'expires':expires,'status':'SIGNED TEST/DEPLOYMENT FILES ONLY; not a running public repository'}
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 for n in ['manifest','payload-root','key','output','base-url']:p.add_argument('--'+n,required=True)
 p.add_argument('--serial',type=int,required=True);p.add_argument('--expires',type=int,required=True);a=p.parse_args()
 print(json.dumps(publish(a.manifest,a.payload_root,a.key,a.output,a.base_url,a.serial,a.expires),indent=2))
