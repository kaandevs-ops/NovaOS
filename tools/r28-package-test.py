#!/usr/bin/env python3
from pathlib import Path
import importlib.util,subprocess,tempfile,hashlib,tarfile,io,json,struct,os
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='packages-',dir=r/'evidence/r28'))
spec=importlib.util.spec_from_file_location('pkg',r/'tools/nova_package_stage.py');pkg=importlib.util.module_from_spec(spec);spec.loader.exec_module(pkg)
c=out/'hello.c';c.write_text('#include <stdio.h>\nint main(void){puts("real external program");return 0;}\n')
subprocess.run(['gcc','-static','-O2',str(c),'-o',str(out/'hello')],check=True)
binary=(out/'hello').read_bytes();root=out/'stage';root.mkdir(mode=0o700);results={}
def archive(name,extra=None,manifest=None,blob=None):
 b=binary if blob is None else blob
 m={'format':1,'id':'nova.hello','version':'1.0.0','arch':'x86_64','sha256':hashlib.sha256(b).hexdigest(),'size':len(b)}
 if manifest:m.update(manifest)
 p=out/(name+'.tar')
 with tarfile.open(p,'w:') as t:
  for n,data in [('manifest.json',json.dumps(m).encode()),('bin/app',b),('LICENSE',b'Test fixture - local source included')]:
   h=tarfile.TarInfo(n);h.size=len(data);h.mode=0o644;t.addfile(h,io.BytesIO(data))
  if extra:
   h,data=extra;t.addfile(h,io.BytesIO(data))
 return p,hashlib.sha256(p.read_bytes()).hexdigest()
def reject(name,p,d,arch='x86_64',dest=root):
 before=sorted(x.name for x in root.iterdir())
 try:pkg.stage(p,d,arch,dest)
 except (ValueError,OSError,tarfile.TarError):results[name]='PASS'
 else:raise AssertionError('accepted '+name)
 assert sorted(x.name for x in root.iterdir())==before
p,d=archive('valid');res=pkg.stage(p,d,'x86_64',root);results['static-package-stage']='PASS'
installed=root/res['directory'];assert (installed/'app').read_bytes()==binary
assert (installed/'app').stat().st_mode&0o777==0o555
reject('no-overwrite',p,d);reject('bad-trust-digest',p,'0'*64);reject('wrong-target',p,d,'arm64')
p,d=archive('bad-manifest',manifest={'permissions':['root']});reject('permission-request',p,d)
p,d=archive('traversal',manifest={'id':'../escape'});reject('manifest-path-traversal',p,d)
h=tarfile.TarInfo('../outside');h.size=1;p,d=archive('extra',(h,b'x'));reject('archive-path-traversal',p,d)
h=tarfile.TarInfo('LICENSE');h.type=tarfile.SYMTYPE;h.linkname='/etc/passwd';p,d=archive('symlink',(h,b''));reject('archive-symlink',p,d)
h=tarfile.TarInfo('LICENSE');h.size=1;p,d=archive('duplicate',(h,b'x'));reject('duplicate-member',p,d)
p,d=archive('oversize-manifest',manifest={'size':True});reject('bool-size',p,d)
b=bytearray(binary);struct.pack_into('<Q',b,32,len(b)+1);p,d=archive('elf-bounds',blob=bytes(b));reject('elf-header-bounds',p,d)
subprocess.run(['gcc',str(c),'-o',str(out/'dynamic')],check=True)
p,d=archive('dynamic',blob=(out/'dynamic').read_bytes());reject('dynamic-runtime-required',p,d)
p,d=archive('valid2');link=out/'link.tar';link.symlink_to(p);reject('archive-file-symlink',link,d)
unsafe=out/'unsafe';unsafe.mkdir(mode=0o755);reject('unsafe-staging-permissions',p,d,dest=unsafe)
# A duplicate JSON field must be rejected independently of archive integrity.
try:pkg.unique([('id','a'),('id','b')])
except pkg.Rejected:results['duplicate-json-field']='PASS'
else:raise AssertionError('duplicate field accepted')
# Inject a write failure: no partly published package or staging directory may remain.
p,d=archive('failure',manifest={'version':'2.0.0'})
original=pkg.os.fsync
calls=[0]
def fail_sync(fd):
 calls[0]+=1
 if calls[0]==1:raise OSError('injected sync failure')
 return original(fd)
pkg.os.fsync=fail_sync
try:reject('write-failure-cleanup',p,d)
finally:pkg.os.fsync=original
assert not any(x.name.startswith('.stage-') for x in root.iterdir())
# Public CLI must reject a mismatched separately supplied digest with a nonzero status.
cli=subprocess.run(['python3',str(r/'tools/nova_package_stage.py'),str(p),'--trusted-sha256','0'*64,'--arch','x86_64','--staging-root',str(root)],capture_output=True,text=True)
assert cli.returncode==1 and 'REJECTED:' in cli.stderr
results['cli-rejection-status']='PASS'

result={'status':'PASS','checks':results,'scope':'build-side archive verification/staging only; payload not executed, no VM installation','path':str(out)}
(r/'evidence/r28/package-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
