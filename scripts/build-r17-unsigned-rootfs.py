#!/usr/bin/env python3
"""Structural initramfs integration test. Never signs, mounts, boots, or reads keys."""
from pathlib import Path
import hashlib,sys,tempfile,shutil,stat,struct,json,gzip,importlib.util
r=Path(__file__).resolve().parents[1];arch=sys.argv[1] if len(sys.argv)>1 else 'x86_64'
(r/'evidence/r17').mkdir(parents=True,exist_ok=True)
assert arch in ['x86_64','arm64'];native=r/f'build/{arch}/r17-native'
out=Path(tempfile.mkdtemp(prefix='unsigned-rootfs.',dir=native));root=out/'rootfs';root.mkdir()
for name in ['dev','proc','sys','run','tmp','Users','Data','Recovery','Trust','System/Core','System/Apps','System/Services','System/Resources','System/Integrity']:(root/name).mkdir(parents=True,exist_ok=True)
shutil.copy2(native/'init',root/'init')
for name in ['Core','Apps']:
 for f in (native/'System'/name).iterdir():shutil.copy2(f,root/'System'/name/f.name)
for f in (r/'configs/services').iterdir():
 if f.is_file():shutil.copy2(f,root/'System/Services'/f.name)
for f in (r/'configs/resources').iterdir():
 if f.is_file():shutil.copy2(f,root/'System/Resources'/f.name)
shutil.copytree(native/'System/Terminal',root/'System/Terminal')
shutil.copytree(native/'System/Network',root/'System/Network')
assert (root/'System/Core/nova-net-broker').is_file()
assert (root/'System/Core/nova-net-setup').is_file()
assert (root/'System/Core/nova-terminal-session').is_file()
assert (root/'System/Core/nova-user-workspace').is_file()
entries=(root/'System/Services/services.list').read_text().splitlines();apps=[];executables={'/init'}
for guest in entries:
 assert guest.startswith('/System/Services/') and '..' not in guest
 manifest=root/guest.lstrip('/');assert manifest.is_file()
 values=dict(line.split('=',1) for line in manifest.read_text().splitlines() if '=' in line and not line.startswith('#'))
 executable=values['exec'];assert executable.startswith('/System/') and '..' not in executable
 p=root/executable.lstrip('/');assert p.is_file() and stat.S_IMODE(p.stat().st_mode)&0o111
 executables.add(executable)
 if values.get('kind')=='application':apps.append(values['app_id'])
assert sorted(apps)==['nova.demo.a','nova.demo.b','nova.files']
for guest in executables:
 header=(root/guest.lstrip('/')).read_bytes()[:64]
 assert header[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',header,18)[0]==(62 if arch=='x86_64' else 183)
 assert struct.unpack_from('<Q',header,24)[0]!=0
manifest=[]
for f in sorted((root/'System').rglob('*')):
 if f.is_file():manifest.append(hashlib.sha256(f.read_bytes()).hexdigest()+'  /'+str(f.relative_to(root)))
(root/'System/Integrity/manifest.sha256').write_text('# UNSIGNED R17 STRUCTURAL TEST ONLY; no release trust or signature\n'+'\n'.join(manifest)+'\n')
spec=importlib.util.spec_from_file_location('nova_cpio_device',r/'scripts/initramfs-device-cpio.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
archive=bytearray()
paths=[root,*sorted(root.rglob('*'))]
for ino,p in enumerate(paths,1):
 name='.' if p==root else str(p.relative_to(root))
 assert not p.is_symlink()
 mode=(stat.S_IFDIR|0o755) if p.is_dir() else (stat.S_IFREG|(0o755 if p.stat().st_mode&0o111 else 0o644))
 archive.extend(module.entry(name,ino=ino,mode=mode,data=b'' if p.is_dir() else p.read_bytes(),mtime=0))
archive.extend(module.entry('TRAILER!!!',ino=len(paths)+1,mode=0,mtime=0))
archive.extend(module.build(0))
# Independently walk actual newc bytes, including concatenated device archive.
index={};off=0
while off<len(archive):
 assert archive[off:off+6]==b'070701';fields=[int(archive[off+6+i*8:off+14+i*8],16) for i in range(13)]
 name_start=off+110;name_end=name_start+fields[11]
 name=archive[name_start:name_end-1].decode();assert archive[name_end-1]==0
 data_start=(name_end+3)&~3;data_end=data_start+fields[6];assert data_end<=len(archive)
 index[name]={'size':fields[6],'mode':fields[1]};off=(data_end+3)&~3
assert 'System/Apps/nova-files' in index and 'System/Core/nova-compositor' in index
assert stat.S_IFMT(index['dev/console']['mode'])==stat.S_IFCHR
assert not (root/'System/Integrity/manifest.sig').exists()
raw=bytes(archive);(out/'initramfs.UNSIGNED.TEST.cpio').write_bytes(raw)
(out/'initramfs.UNSIGNED.TEST.cpio.gz').write_bytes(gzip.compress(raw,mtime=0))
result={'status':'STRUCTURAL_TEST_PASS_NOT_RELEASE','architecture':arch,'directory':str(out),'applications':apps,'executables_verified':len(executables),'cpio_entries':len(index),'cpio_sha256':hashlib.sha256(raw).hexdigest(),'signature':'ABSENT_INTENTIONALLY','trust':'UNPINNED; boot/production acceptance pending'}
(out/'result.json').write_text(json.dumps(result,indent=2));(r/f'evidence/r17/unsigned-rootfs-{arch}.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
