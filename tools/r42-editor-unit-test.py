from pathlib import Path
import os,subprocess,tempfile,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r42';e.mkdir(parents=True,exist_ok=True);out=Path(tempfile.mkdtemp(prefix='editor-unit-',dir=e));out.chmod(0o755)
results={}
for kind,cc,flags in [('asan','clang',['-fsanitize=address,undefined','-g']),('policy','gcc',[])]:
 exe=out/kind;subprocess.run([cc,'-std=c11','-O1','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function',*flags,str(r/'tests/r42-editor-store.c'),'-o',str(exe)],check=True)
 home=out/(kind+'-home');home.mkdir(mode=0o700);os.chown(home,12004,12004)
 cmd=['setpriv','--reuid=12004','--regid=12004','--clear-groups','--no-new-privs',str(exe)]+(['policy'] if kind=='policy' else [])
 with (out/(kind+'.log')).open('w') as log:p=subprocess.run(cmd,cwd=home,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 results[kind]={'status':'PASS' if not p.returncode else 'FAIL','exit':p.returncode,'log':str(out/(kind+'.log'))};print(kind,results[kind],flush=True)
(e/'editor-unit-results.json').write_text(json.dumps(results,indent=2));assert all(v['exit']==0 for v in results.values()),results
