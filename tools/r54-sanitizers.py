from pathlib import Path
import subprocess,tempfile,os,json
r=Path(__file__).resolve().parents[1];out=r/'evidence/r54'
with tempfile.TemporaryDirectory(prefix='r54-asan-') as td:
 p=Path(td);p.chmod(0o755);home=p/'home';home.mkdir(mode=0o700);exe=p/'test'
 subprocess.run(['clang','-std=c11','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Wno-misleading-indentation','-I'+str(r/'src/terminal'),str(r/'tests/r54-desktop-files.c'),'-o',str(exe)],check=True)
 run=subprocess.run([str(exe),str(home)],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},timeout=90)
 result=dict(scope='ASan/UBSan storage operations; LeakSanitizer disabled across setuid; separate FD-cycle test',passed=run.returncode==0,stdout=run.stdout,stderr=run.stderr)
 (out/'desktop-files-sanitizers.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));assert run.returncode==0
