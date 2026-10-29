from pathlib import Path
import subprocess,json,struct,hashlib,os
r=Path(__file__).resolve().parents[1];e=r/'evidence/r25/build';e.mkdir(parents=True,exist_ok=True);results={}
for arch,machine in [('x86_64',62),('arm64',183)]:
 for name,cmd in [('native',['bash','scripts/build-r17-native.sh',arch]),('unsigned-initramfs',['python3','scripts/build-r17-unsigned-rootfs.py',arch])]:
  with (e/(arch+'-'+name+'.log')).open('w') as log:p=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
  results[arch+'-'+name]={'status':'PASS' if p.returncode==0 else 'FAIL','exit_code':p.returncode};print(arch,name,results[arch+'-'+name],flush=True)
  (e/'results.json').write_text(json.dumps(results,indent=2))
  if p.returncode:raise SystemExit(p.returncode)
 base=r/f'build/{arch}/r17-native';files=[base/'init',*sorted((base/'System/Core').glob('*')),*sorted((base/'System/Apps').glob('*')),base/'System/Terminal/bin/busybox']
 assert len(files)==11
 checks={}
 for f in files:
  data=f.read_bytes();assert data[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',data,18)[0]==machine
  off=struct.unpack_from('<Q',data,32)[0];size,count=struct.unpack_from('<HH',data,54)
  kinds=[struct.unpack_from('<I',data,off+i*size)[0] for i in range(count)];assert 2 not in kinds and 3 not in kinds
  checks[str(f.relative_to(r))]={'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'machine':machine,'static':True}
 (e/(arch+'-elf.json')).write_text(json.dumps(checks,indent=2))
identity=subprocess.check_output(['python3','tools/r24-build-id.py'],cwd=r,text=True).strip()
for arch in ['x86_64','arm64']:assert identity.encode() in (r/f'build/{arch}/r17-native/System/Core/nova-compositor').read_bytes()
(e/'identity.json').write_text(json.dumps({'identity':identity,'both_arch_match':True},indent=2));print('22 static ELF + both compositor/source identities verified',identity,flush=True)
