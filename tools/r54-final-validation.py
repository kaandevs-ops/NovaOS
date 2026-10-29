from pathlib import Path
import subprocess,json,datetime,os
r=Path(__file__).resolve().parents[1];out=r/'evidence/r54/final-validation';out.mkdir(parents=True,exist_ok=True)
checks=[('desktop-storage',['python3','tools/r54-desktop-files-test.py']),('desktop-ui',['python3','tools/r54-desktop-ui-test.py']),('r53-dock-drag-regression',['python3','tools/r54-r53-dock-regression.py']),('security-regressions',['python3','tools/r54-security-regression.py']),('app-stage-negative',['python3','tools/r54-app-stage-regression.py']),('component-builds',['python3','tools/r54-build-components.py'])]
results={}
for name,cmd in checks:
 print('START',name,flush=True)
 with (out/(name+'.log')).open('w') as log:
  try:
   process=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,timeout=240,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
   results[name]=dict(status='PASS' if process.returncode==0 else 'FAIL',returncode=process.returncode)
  except subprocess.TimeoutExpired:results[name]=dict(status='FAIL',reason='timeout')
 print(name,results[name],flush=True)
 if results[name]['status']=='FAIL':print((out/(name+'.log')).read_text()[-5000:],flush=True)
 (out/'results.json').write_text(json.dumps(dict(scope='SOURCE/HOST ONLY; NO GUEST',results=results),indent=2))
raise SystemExit(any(row['status']!='PASS' for row in results.values()))
