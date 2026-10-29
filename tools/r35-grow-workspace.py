#!/usr/bin/env python3
"""Offline 16MiB -> 256MiB migration to a NEW file. Never modifies source.
Requires an unmounted, isolated exported home image, not a live VM disk.
The output is a filesystem image, NOT a GPT disk or production updater.
"""
import argparse,fcntl,hashlib,json,os,stat,subprocess,tempfile
from pathlib import Path
OLD=16*1024*1024
NEW=256*1024*1024

def digest(fd):
 os.lseek(fd,0,0);h=hashlib.sha256()
 while True:
  b=os.read(fd,1024*1024)
  if not b:break
  h.update(b)
 return h.hexdigest()

def grow(source,destination):
 source=Path(source);destination=Path(destination)
 # Caller must supply a private, existing output directory. No symlink parents.
 parent=destination.parent.resolve(strict=True)
 if destination.parent.absolute()!=parent:raise ValueError('symlink output directory rejected')
 ps=parent.stat()
 if ps.st_uid!=os.geteuid() or stat.S_IMODE(ps.st_mode)!=0o700:raise ValueError('private 0700 output directory required')
 if os.path.lexists(destination):raise FileExistsError(destination)
 src=os.open(source,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK|os.O_CLOEXEC)
 staging=None
 try:
  st=os.fstat(src)
  if not stat.S_ISREG(st.st_mode) or st.st_size!=OLD or st.st_nlink!=1:raise ValueError('single-link 16MiB regular source required')
  if st.st_uid!=os.geteuid() or stat.S_IMODE(st.st_mode)!=0o600:raise ValueError('private source owner/mode required')
  fcntl.flock(src,fcntl.LOCK_EX|fcntl.LOCK_NB)
  before=digest(src)
  fd,name=tempfile.mkstemp(prefix='.grow-',dir=parent);staging=Path(name)
  try:
   os.posix_fallocate(fd,0,NEW);os.lseek(src,0,0)
   while True:
    chunk=os.read(src,1024*1024)
    if not chunk:break
    view=memoryview(chunk)
    while view:
     n=os.write(fd,view)
     if n<=0:raise OSError('short write')
     view=view[n:]
   os.fsync(fd)
  finally:os.close(fd)
  # Resize only a clean private copy. Corruption is rejected, never repaired.
  subprocess.run(['e2fsck','-fn',str(staging)],check=True,capture_output=True)
  # Record a forced clean check on the PRIVATE copy: resize2fs requires
  # this after a previously mounted filesystem, even when -fn reports clean.
  subprocess.run(['e2fsck','-fp',str(staging)],check=True,capture_output=True)
  subprocess.run(['resize2fs',str(staging)],check=True,capture_output=True)
  subprocess.run(['e2fsck','-fn',str(staging)],check=True,capture_output=True)
  if digest(src)!=before:raise ValueError('source changed during migration')
  with staging.open('rb') as f:os.fsync(f.fileno())
  after=hashlib.sha256(staging.read_bytes()).hexdigest()
  # Atomic NOREPLACE publication; an existing destination is never overwritten.
  os.link(staging,destination,follow_symlinks=False)
  staging.unlink();staging=None
  d=os.open(parent,os.O_RDONLY|os.O_DIRECTORY)
  try:os.fsync(d)
  finally:os.close(d)
  return {'status':'PASS','source_sha256':before,'output_sha256':after,'source_bytes':OLD,'output_bytes':NEW,'scope':'offline filesystem copy; no guest acceptance'}
 finally:
  os.close(src)
  if staging is not None:staging.unlink(missing_ok=True)

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('source');p.add_argument('destination');a=p.parse_args()
 print(json.dumps(grow(a.source,a.destination),indent=2))
