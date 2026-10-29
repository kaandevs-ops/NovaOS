from pathlib import Path
import tempfile,subprocess,os,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r41';out=Path(tempfile.mkdtemp(prefix='broker-pages-',dir=e));home=out/'home';home.mkdir();(home/'Documents').mkdir();(home/'note.txt').write_text('shared-home\n');(home/'Documents/inside.txt').write_text('inside\n');(home/'escape').symlink_to('/etc',target_is_directory=True)
for i in range(200):(home/f'Class-{i:03d}.txt').write_text('real page target\n')
for p in [home,home/'Documents',home/'Documents/inside.txt',home/'note.txt',*home.glob('Class-*')]:os.chown(p,12004,12004);p.chmod(0o700 if p.is_dir() else 0o600)
cmd=['clang','-std=c11','-O2','-g','-ffunction-sections','-fdata-sections','-Wno-unused-function',f'-DNOVA_HOME_SANDBOX_SOURCE="{home}"','-I'+str(r/'src/runtime'),f'-DNOVA_LEGACY_NOVAD="{r.parent}/phase19-desktop-r40-files-navigation-candidate/src/init/main.c"',str(r/'tests/r41-posix-worker.c'),str(r/'src/runtime/nova_runtime.c'),str(r/'files/src/nova_files_core.c'),str(r/'files/src/nova_files_memory.c'),'-Wl,--gc-sections','-o',str(out/'test')]
with (out/'build.log').open('w') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
with (out/'run.log').open('w') as log:subprocess.run([str(out/'test'),'10'],stdout=log,stderr=subprocess.STDOUT,timeout=60,check=True)
record={'status':'PASS','scope':'real UID3 Files seccomp, UID12004 home worker, credential/FD IPC, NEW Files client with untouched R40 worker: capability absence, size/preview/navigation preserved, POSIX Unavailable, 100 metadata FD stability and worker cleanup; WSL NOT VM','path':str(out)}
(e/'legacy-worker-result.json').write_text(json.dumps(record,indent=2));print(json.dumps(record))
