from pathlib import Path
import json,struct,hashlib,platform,subprocess
r=Path(__file__).resolve().parents[1];out={}
for arch,machine in [('x86_64',62),('arm64',183)]:
 base=r/'build'/arch/'r17-native';paths=[base/'init',*sorted((base/'System/Core').glob('*')),*sorted((base/'System/Apps').glob('*'))];assert len(paths)==9
 for p in paths:
  b=p.read_bytes();assert b[:6]==b'\x7fELF\x02\x01';actual=struct.unpack_from('<H',b,18)[0];assert actual==machine
  phoff=struct.unpack_from('<Q',b,32)[0];entsize,num=struct.unpack_from('<HH',b,54);types=[struct.unpack_from('<I',b,phoff+i*entsize)[0] for i in range(num)];assert 3 not in types and 2 not in types
  out[str(p.relative_to(r))]={'sha256':hashlib.sha256(b).hexdigest(),'machine':actual,'static_no_interp':True,'bytes':len(b)}
(r/'evidence/r22/native-binary-verification.json').write_text(json.dumps(out,indent=2))
(r/'evidence/r22/environment.json').write_text(json.dumps({'platform':platform.platform(),'kernel':platform.release(),'cpu':subprocess.check_output(['lscpu'],text=True),'clang':subprocess.check_output(['clang','--version'],text=True),'scope':'WSL host tools; no guest boot'},indent=2));print('18 static ELF verified')
