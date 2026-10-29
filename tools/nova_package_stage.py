#!/usr/bin/env python3
"""Offline build-side package staging. Does not execute/install into a live guest.
The archive digest MUST come from a separately trusted channel, not its manifest.
No scripts, dependencies, devices, symlinks, dynamic ELF or permission requests.
"""
import argparse,ctypes,errno,hashlib,io,json,os,re,secrets,stat,struct,tarfile
MAX_ARCHIVE=12*1024*1024
MAX_BINARY=8*1024*1024
class Rejected(ValueError):pass
def require(ok,why):
 if not ok:raise Rejected(why)
def unique(pairs):
 d={}
 for k,v in pairs:
  require(k not in d,'duplicate manifest field');d[k]=v
 return d
def validate(data,digest,arch):
 require(arch in ('x86_64','arm64'),'unsupported target architecture')
 require(re.fullmatch('[0-9a-f]{64}',digest) is not None,'invalid trusted digest')
 require(len(data)<=MAX_ARCHIVE,'archive too large')
 require(hashlib.sha256(data).hexdigest()==digest,'trusted archive digest mismatch')
 entries={}
 with tarfile.open(fileobj=io.BytesIO(data),mode='r:') as t:
  for member in t:
   require(member.name in ('manifest.json','bin/app','LICENSE'),'unexpected archive path')
   require(member.name not in entries,'duplicate archive path')
   require(member.isfile() and not member.sparse,'only regular non-sparse files allowed')
   require(member.mode & 0o7000 == 0,'special permission bits forbidden')
   limit=MAX_BINARY if member.name=='bin/app' else 65536
   require(0<=member.size<=limit,'member too large')
   entries[member.name]=t.extractfile(member).read(limit+1)
   require(len(entries[member.name])==member.size,'truncated member')
 require(set(entries)=={'manifest.json','bin/app','LICENSE'},'missing package member')
 m=json.loads(entries['manifest.json'],object_pairs_hook=unique)
 require(type(m) is dict and set(m)=={'format','id','version','arch','sha256','size'},'manifest schema mismatch')
 require(type(m['format']) is int and m['format']==1,'unsupported format')
 require(type(m['id']) is str and re.fullmatch('nova[.][a-z][a-z0-9-]{0,47}',m['id']) is not None,'invalid application ID')
 require(type(m['version']) is str and re.fullmatch('[0-9]{1,4}[.][0-9]{1,4}[.][0-9]{1,4}',m['version']) is not None,'invalid version')
 require(m['arch']==arch,'architecture mismatch')
 binary=entries['bin/app']
 require(type(m['size']) is int and m['size']==len(binary),'binary size mismatch')
 require(m['sha256']==hashlib.sha256(binary).hexdigest(),'binary hash mismatch')
 require(len(binary)>=64 and binary[:7]==b'\x7fELF\x02\x01\x01','ELF64 little endian required')
 require(struct.unpack_from('<HH',binary,16)==(2,62 if arch=='x86_64' else 183),'static executable target mismatch')
 phoff=struct.unpack_from('<Q',binary,32)[0];entsz,count=struct.unpack_from('<HH',binary,54)
 require(entsz==56 and 0<count<=128 and phoff+count*entsz<=len(binary),'invalid ELF program header bounds')
 loads=0;total_memory=0
 for i in range(count):
  typ,flags,offset,va,pa,filesz,memsz,align=struct.unpack_from('<IIQQQQQQ',binary,phoff+i*entsz)
  require(typ not in (2,3),'dynamic ELF or interpreter forbidden')
  require(offset+filesz<=len(binary),'ELF segment out of bounds')
  if typ==1:
   require(filesz<=memsz and memsz<=64*1024*1024,'ELF memory bound exceeded')
   require(flags&3!=3,'writable executable segment forbidden');loads+=1;total_memory+=memsz
   require(total_memory<=64*1024*1024,'total ELF memory bound exceeded')
 require(loads>0,'missing load segment')
 require(bool(entries['LICENSE'].strip()),'license text required')
 return m,entries

def stage(archive,digest,arch,destination):
 # Pin opened archive; do not follow a last-component symlink or stream devices.
 fd=os.open(archive,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
 try:
  st=os.fstat(fd);require(stat.S_ISREG(st.st_mode) and st.st_size<=MAX_ARCHIVE,'invalid archive file')
  with os.fdopen(os.dup(fd),'rb') as f:data=f.read(MAX_ARCHIVE+1)
 finally:os.close(fd)
 m,entries=validate(data,digest,arch)
 root=os.open(destination,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
 tmp='.stage-'+secrets.token_hex(16);created=False;sub=None
 try:
  st=os.fstat(root);require(st.st_uid==os.geteuid() and stat.S_IMODE(st.st_mode)==0o700,'staging root must be caller-owned mode 0700')
  os.mkdir(tmp,0o700,dir_fd=root);created=True
  sub=os.open(tmp,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW,dir_fd=root)
  files={'app':entries['bin/app'],'LICENSE':entries['LICENSE'],'manifest.json':json.dumps(m,sort_keys=True).encode(),'archive.sha256':digest.encode()+b'\n'}
  for name,content in files.items():
   f=os.open(name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600,dir_fd=sub)
   try:
    with os.fdopen(os.dup(f),'wb') as stream:stream.write(content);stream.flush()
    os.fchmod(f,0o555 if name=='app' else 0o444);os.fsync(f)
   finally:os.close(f)
  os.fchmod(sub,0o555);os.fsync(sub)
  target=m['id']+'-'+m['version']+'-'+arch
  # Atomic publication, fail on ANY existing target. No overwrite or merge.
  libc=ctypes.CDLL(None,use_errno=True)
  require(hasattr(libc,'renameat2'),'atomic no-replace rename unavailable')
  rc=libc.renameat2(root,tmp.encode(),root,target.encode(),1)
  if rc:raise OSError(ctypes.get_errno(),'atomic publication rejected')
  created=False;os.fsync(root)
  return {'status':'STAGED','id':m['id'],'version':m['version'],'arch':arch,'directory':target,'archive_sha256':digest,'guest_installation':'PENDING','execution':'NOT_PERFORMED'}
 finally:
  if created and sub is not None:
   os.fchmod(sub,0o700)
   for name in os.listdir(sub):os.unlink(name,dir_fd=sub)
  if sub is not None:os.close(sub)
  if created:os.rmdir(tmp,dir_fd=root)
  os.close(root)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive');p.add_argument('--trusted-sha256',required=True);p.add_argument('--arch',choices=['x86_64','arm64'],required=True);p.add_argument('--staging-root',required=True)
 a=p.parse_args()
 try:print(json.dumps(stage(a.archive,a.trusted_sha256,a.arch,a.staging_root),indent=2))
 except (Rejected,OSError,ValueError,tarfile.TarError) as e:p.exit(1,'REJECTED: '+str(e)+'\n')
