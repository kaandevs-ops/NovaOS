from pathlib import Path
import subprocess,json,os
r=Path(__file__).resolve().parents[1];e=r/'evidence/r43';e.mkdir(parents=True,exist_ok=True)
for name in ('r41','r38','r40'):(r/'evidence'/name).mkdir(exist_ok=True)
steps=['tools/r42-validate.py','tools/r42-terminal-session-regression.py','tools/r42-terminal-desktop-regression.py','tools/r43-reader-unit-test.py','tools/r43-reader-pty-test.py','tools/r43-reader-desktop-test.py'];results={}
source_id=subprocess.check_output(['python3','tools/r24-build-id.py'],cwd=r,text=True).strip()
for script in steps:
 print('START',script,flush=True)
 with (e/(Path(script).stem+'.log')).open('w') as log:p=subprocess.run(['python3',script],cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'NOVA_R38_WEEK4_LAB':'1'})
 results[script]={'status':'PASS' if p.returncode==0 else 'FAIL','exit':p.returncode};(e/'validation-results.json').write_text(json.dumps(results,indent=2));print(script,results[script],flush=True)
 if p.returncode:raise SystemExit(p.returncode)
assert source_id==subprocess.check_output(['python3','tools/r24-build-id.py'],cwd=r,text=True).strip()
(e/'source-frozen-id.txt').write_text(source_id+'\n')
