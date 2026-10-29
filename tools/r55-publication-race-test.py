from pathlib import Path
import tempfile,subprocess,shutil,os,json
r=Path(__file__).resolve().parents[1]
if os.geteuid()!=0:raise SystemExit('Isolated chroot fixture requires root')
out=Path(tempfile.mkdtemp(prefix='publication-race-',dir=r/'evidence/r55'));root=out/'root'
for n in ['bin','etc/nova-packages','home/nova']:(root/n).mkdir(parents=True,exist_ok=True)
(root/'home/nova').chmod(0o700);os.chown(root/'home/nova',12004,12004)
shutil.copyfile(r/'build/x86_64/r25-terminal/System/Terminal/etc/nova-packages/2048',root/'etc/nova-packages/2048')
subprocess.run(['gcc','-std=c11','-O2','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',f'-DNOVA_PACKAGE_CATALOG="{r}/build/x86_64/r33-catalog/catalog.h"',str(r/'tests/r55-package-publication-race.c'),str(r/'src/runtime/nova_sha256.c'),'-o',str(root/'bin/probe')],check=True)
x=subprocess.run(['chroot','--userspec=12004:12004',str(root),'/bin/probe','install','2048'],capture_output=True,text=True);(out/'test.log').write_text(x.stdout+x.stderr)
assert x.returncode==1 and 'File exists' in x.stderr
assert (root/'home/nova/.nova-apps/2048').read_bytes()==b'RACE_SENTINEL'
assert not list((root/'home/nova/.nova-apps').glob('.install-*'))
record={'status':'PASS','scope':'deterministic host syscall-boundary race, real renameat2 and UID12004; not guest','path':str(out)}
(r/'evidence/r55/publication-race.json').write_text(json.dumps(record,indent=2));print(record)
