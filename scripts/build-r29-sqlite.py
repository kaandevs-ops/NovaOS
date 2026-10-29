#!/usr/bin/env python3
"""Pinned upstream SQLite -> static ELF -> verified offline package -> terminal bundle."""
from pathlib import Path
import hashlib,io,json,os,shutil,subprocess,sys,tarfile,zipfile
r=Path(__file__).resolve().parents[1];arch=sys.argv[1]
assert arch in ('x86_64','arm64')
source=r/'third_party/sqlite/sqlite-amalgamation-3540100.zip'
assert hashlib.sha3_256(source.read_bytes()).hexdigest()=='92281c86e2957f9b9305890f9ac04204caa7b677cc868dc326d942689df28e30'
out=r/f'build/{arch}/r29-sqlite';out.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(source) as z:
 for name in ('sqlite3.c','sqlite3.h','shell.c'):
  (out/name).write_bytes(z.read('sqlite-amalgamation-3540100/'+name))
cc='gcc' if arch=='x86_64' else 'aarch64-linux-gnu-gcc'
cmd=[cc,'-O2','-static','-DSQLITE_THREADSAFE=0','-DSQLITE_OMIT_LOAD_EXTENSION','-DSQLITE_DQS=0','-DSQLITE_DEFAULT_MEMSTATUS=0','-DSQLITE_MAX_MMAP_SIZE=0','shell.c','sqlite3.c','-lm','-o',str(out/'sqlite3')]
with (out/'compile.log').open('w') as log:subprocess.run(cmd,cwd=out,check=True,stdout=log,stderr=subprocess.STDOUT)
binary=(out/'sqlite3').read_bytes();sha=hashlib.sha256(binary).hexdigest()
m={'format':1,'id':'nova.sqlite','version':'3.54.1','arch':arch,'sha256':sha,'size':len(binary)}
license_text=b'SQLite 3.54.1 - upstream source: https://www.sqlite.org/2026/sqlite-amalgamation-3540100.zip\n'+(out/'sqlite3.c').read_bytes()[:1000]
pkg=out/'nova.sqlite.tar'
with tarfile.open(pkg,'w:') as t:
 for name,data in [('manifest.json',json.dumps(m).encode()),('bin/app',binary),('LICENSE',license_text)]:
  h=tarfile.TarInfo(name);h.size=len(data);h.mode=0o644;h.mtime=0;t.addfile(h,io.BytesIO(data))
# Digest derives from this pinned-source local build, not an untrusted downloaded manifest.
digest=hashlib.sha256(pkg.read_bytes()).hexdigest();stage=out/'verified';stage.mkdir(mode=0o700,exist_ok=True)
sys.path.insert(0,str(r/'tools'));from nova_package_stage import stage as verify_stage
name='nova.sqlite-3.54.1-'+arch
if (stage/name).exists():
 # Existing build output is never overwritten; verify all bytes against this build.
 assert (stage/name/'app').read_bytes()==binary
 assert (stage/name/'archive.sha256').read_text().strip()==digest
else:verify_stage(pkg,digest,arch,stage)
binroot=r/f'build/{arch}/r25-terminal/System/Terminal/bin';binroot.mkdir(parents=True,exist_ok=True)
dest=binroot.parent/'etc/nova-packages/sqlite3'
dest.parent.mkdir(parents=True,exist_ok=True)
if dest.exists() or dest.is_symlink():
 assert not dest.is_symlink() and dest.read_bytes()==binary,'refuse existing executable replacement'
else:
 with dest.open('xb') as f:f.write(binary);f.flush();os.fsync(f.fileno())
 dest.chmod(0o555)
# Keep the R30 direct CLI working; the managed install has a separate lifecycle.
alias=binroot/'sqlite3'
if not alias.exists() and not alias.is_symlink():alias.symlink_to('../etc/nova-packages/sqlite3')
else:assert alias.is_symlink() and str(alias.readlink())=='../etc/nova-packages/sqlite3'

sources=binroot.parent/'Source/sqlite';sources.mkdir(parents=True,exist_ok=True)
shutil.copy2(source,sources/source.name);(sources/'NOTICE.txt').write_bytes(license_text)
result={'status':'BUILD_PASS','arch':arch,'upstream':'3.54.1','binary_sha256':sha,'archive_sha256':digest,'package_verified':True,'guest_boot':'PENDING','network':'NOT_GRANTED','load_extensions':'DISABLED','recipe':cmd}
(out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
