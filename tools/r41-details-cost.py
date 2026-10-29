from pathlib import Path
import tempfile,subprocess,os,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r41';out=Path(tempfile.mkdtemp(prefix='broker-pages-',dir=e));home=out/'home';home.mkdir();(home/'Documents').mkdir();(home/'note.txt').write_text('shared-home\n');(home/'Documents/inside.txt').write_text('inside\n');(home/'escape').symlink_to('/etc',target_is_directory=True)
for i in range(200):(home/f'Class-{i:03d}.txt').write_text('real page target\n')
for p in [home,home/'Documents',home/'Documents/inside.txt',home/'note.txt',*home.glob('Class-*')]:os.chown(p,12004,12004);p.chmod(0o700 if p.is_dir() else 0o600)
cmd=['clang','-std=c11','-O2','-g','-ffunction-sections','-fdata-sections','-Wno-unused-function',f'-DNOVA_HOME_SANDBOX_SOURCE="{home}"','-I'+str(r/'src/runtime'),str(r/'tests/r41-details-cost.c'),str(r/'src/runtime/nova_runtime.c'),str(r/'files/src/nova_files_core.c'),str(r/'files/src/nova_files_memory.c'),'-Wl,--gc-sections','-o',str(out/'test')]
with (out/'build.log').open('w') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
with (out/'run.log').open('w') as log:subprocess.run([str(out/'test')],stdout=log,stderr=subprocess.STDOUT,timeout=60,check=True)
metrics=json.loads((out/'run.log').read_text());record={'status':'PASS','scope':'same real UID3 Files seccomp and UID12004 worker/channel, same file, alternating legacy40/details64 requests, 20 warmup +500 samples; WSL host not VM','metrics':metrics};(e/'details-ipc-performance.json').write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2))
