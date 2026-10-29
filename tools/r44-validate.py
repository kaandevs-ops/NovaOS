from pathlib import Path
import subprocess,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r44/final';e.mkdir(parents=True,exist_ok=True)
gcc=['gcc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation']
steps=[('inherited-r43',['python3','tools/r43-validate.py']),
 ('sixel-model-build',gcc+['-fsanitize=address,undefined','tests/r44-pdf-model.c','-o',str(e/'model')]),('sixel-model-asan-ubsan',[str(e/'model')]),
 ('policy-build',gcc+['tests/r44-pdf-policy.c','-o',str(e/'policy')]),('stacked-policy',[str(e/'policy')]),
 ('fault-worker-build',gcc+['tests/r44-pdf-fault-worker.c','-o',str(e/'fault-worker')]),
 ('supervisor-build',gcc+['-fsanitize=address,undefined','-DNOVA_PDF_WORKER="'+str(e/'fault-worker')+'"','tests/r44-pdf-supervisor.c','-o',str(e/'supervisor')]),('supervisor-faults',[str(e/'supervisor')]),
 ('real-pdf-worker',['python3','tools/r44-pdf-worker-test.py']),('real-pdf-pty',['python3','tools/r44-pdf-pty-test.py']),('native-pdf-desktop',['python3','tools/r44-pdf-desktop-test.py'])]
source=subprocess.check_output(['python3','tools/r24-build-id.py'],cwd=r,text=True).strip();results={}
for name,cmd in steps:
 print('START',name,flush=True)
 with (e/(name+'.log')).open('w') as log:p=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,timeout=900)
 results[name]={'status':'PASS' if p.returncode==0 else 'FAIL','exit':p.returncode};(e/'results.json').write_text(json.dumps(results,indent=2));print(name,results[name],flush=True)
 if p.returncode:raise SystemExit(p.returncode)
assert source==subprocess.check_output(['python3','tools/r24-build-id.py'],cwd=r,text=True).strip()
(e/'source-id.txt').write_text(source+'\n')
