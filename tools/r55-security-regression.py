from pathlib import Path
import subprocess,json,os
r=Path(__file__).resolve().parents[1];out=r/'evidence/r55/security';out.mkdir(parents=True,exist_ok=True);results={}
for name,compositor in [('r50-terminal-policy',False),('r50-path-drop',False),('r17-buffer-safety',True),('r24-input-damage',True),('r16-catalog-runtime',True)]:
 exe=out/name
 cmd=['clang','-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections','-Isrc/runtime','-Isrc/terminal','tests/'+name+'.c']
 if compositor:cmd+=['src/runtime/nova_runtime.c','src/runtime/nova_font.c']
 cmd+=['-Wl,--gc-sections','-o',str(exe)]
 with (out/(name+'.log')).open('w') as log:
  build=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
  test=subprocess.run([str(exe)],cwd=r,stdout=log,stderr=subprocess.STDOUT,timeout=90) if build.returncode==0 else None
 results[name]={'build':build.returncode,'test':None if test is None else test.returncode,'status':'PASS' if test is not None and test.returncode==0 else 'FAIL'}
 print(name,results[name],flush=True)
 (out/'results.json').write_text(json.dumps(results,indent=2))
 if results[name]['status']=='FAIL':print((out/(name+'.log')).read_text()[-4000:])
assert all(x['status']=='PASS' for x in results.values())
