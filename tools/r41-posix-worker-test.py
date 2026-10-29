from pathlib import Path
import tempfile,subprocess,os,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r41';out=Path(tempfile.mkdtemp(prefix='broker-pages-',dir=e));home=out/'home';home.mkdir();(home/'Documents').mkdir();(home/'note.txt').write_text('shared-home\n');(home/'Documents/inside.txt').write_text('inside\n');(home/'escape').symlink_to('/etc',target_is_directory=True);os.mkfifo(home/'pipe',0o600);os.chown(home/'pipe',12004,12004)
for i in range(200):(home/f'Class-{i:03d}.txt').write_text('real page target\n')
for p in [home,home/'Documents',home/'Documents/inside.txt',home/'note.txt',*home.glob('Class-*')]:os.chown(p,12004,12004);p.chmod(0o700 if p.is_dir() else 0o600)
cmd=['clang','-std=c11','-O2','-g','-ffunction-sections','-fdata-sections','-Wno-unused-function',f'-DNOVA_HOME_SANDBOX_SOURCE="{home}"','-I'+str(r/'src/runtime'),str(r/'tests/r41-posix-worker.c'),str(r/'src/runtime/nova_runtime.c'),str(r/'files/src/nova_files_core.c'),str(r/'files/src/nova_files_memory.c'),'-Wl,--gc-sections','-o',str(out/'test')]
with (out/'build.log').open('w') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
for mode in (9,12):
 with (out/(str(mode)+'-run.log')).open('w') as log:subprocess.run([str(out/'test'),str(mode)],stdout=log,stderr=subprocess.STDOUT,timeout=60,check=True)
record={'status':'PASS','scope':'real UID3 Files seccomp, UID12004 home worker, credential/FD IPC, real chmod/chown transitions, POSIX metadata, unchanged 40-byte legacy META, DAC denial, DETAILS traversal/symlink/FIFO rejection, 1000 details FD stability and worker cleanup; WSL NOT VM','path':str(out)}
(e/'posix-worker-result.json').write_text(json.dumps(record,indent=2));print(json.dumps(record))
