from pathlib import Path
import tempfile,subprocess,os,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r26';e.mkdir(parents=True,exist_ok=True);out=Path(tempfile.mkdtemp(prefix='worker-',dir=e));home=out/'home';home.mkdir();(home/'Documents').mkdir();(home/'note.txt').write_text('shared-home\n');(home/'Documents/inside.txt').write_text('inside\n');(home/'escape').symlink_to('/etc',target_is_directory=True)
for p in [home,home/'Documents',home/'note.txt',home/'Documents/inside.txt']:
 os.chown(p,12004,12004);p.chmod(0o700 if p.is_dir() else 0o600)
cmd=['clang','-std=c11','-O2','-g','-ffunction-sections','-fdata-sections','-Wno-unused-function',f'-DNOVA_HOME_SANDBOX_SOURCE="{home}"','-I'+str(r/'src/runtime'),str(r/'tests/r26-home-worker.c'),str(r/'src/runtime/nova_runtime.c'),str(r/'files/src/nova_files_core.c'),str(r/'files/src/nova_files_memory.c'),'-Wl,--gc-sections','-o',str(out/'test')]
with (out/'build.log').open('w') as f:subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,check=True)
results={}
for mode,name in enumerate(['read-only-shared-home-and-1000-cycles','traversal-rejected','sequence-replay-rejected','unexpected-fd-rejected','different-pid-same-uid-rejected','wrong-uid-rejected','stamp-name-rejected','unknown-opcode-rejected']):
 with (out/(name+'.log')).open('w') as f:subprocess.run([str(out/'test'),str(mode)],stdout=f,stderr=subprocess.STDOUT,check=True,timeout=30)
 results[name]='PASS'
(out/'results.json').write_text(json.dumps(results,indent=2));(e/'worker-result.json').write_text(json.dumps({'status':'PASS','path':str(out),'checks':results,'scope':'real UID3 Files seccomp + UID12004 worker, private kernel credentials and FD IPC in WSL; not VM'},indent=2));print(results)
