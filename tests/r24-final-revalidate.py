from pathlib import Path
import subprocess,json,fcntl,shutil,os
r=Path(__file__).resolve().parents[1];e=r/'evidence/r24';lock=(e/'validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
checks=[('security-safe-suite','tests/run-stage5a-phase18-step3-r2-safe-suite.sh'),('frozen-step7-suite','tests/run-stage5a-phase18-step7-r2-package-suite.sh'),('r24-input-damage-properties-diagnostics','tests/r24-units.sh')];results={};inherited=json.loads((r/'evidence/results.json').read_text());shutil.copy2(r/'evidence/results.json',e/'r16-before-revalidation.json')
for name,script in checks:
 print('START',name,flush=True)
 with (e/('revalidated-'+name+'.log')).open('w') as f:p=subprocess.run(['bash',script],cwd=r,stdout=f,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
 result={'status':'PASS' if p.returncode==0 else 'FAIL','exit_code':p.returncode,'log':'evidence/r24/revalidated-'+name+'.log'};results[name]=result;print(name,result,flush=True)
 if name in inherited:inherited[name]=result
(r/'evidence/results.json').write_text(json.dumps(inherited,indent=2));a=json.loads((r/'evidence/r17/results.json').read_text());a['r16-inherited-regressions']={'status':'PASS' if all(x['status']=='PASS' for x in inherited.values()) else 'FAIL','exit_code':0 if all(x['status']=='PASS' for x in inherited.values()) else 1};(r/'evidence/r17/results.json').write_text(json.dumps(a,indent=2));p=e/'local-results.json';d=json.loads(p.read_text());shutil.copy2(p,e/'local-before-revalidation.json');d['checks']['inherited-r17']={'status':'PASS' if all(x['status']=='PASS' for x in a.values()) else 'FAIL','exit_code':0 if all(x['status']=='PASS' for x in a.values()) else 1};d['checks']['r24-input-damage-properties-diagnostics']=results['r24-input-damage-properties-diagnostics'];p.write_text(json.dumps(d,indent=2));(e/'revalidation-results.json').write_text(json.dumps(results,indent=2));assert all(x['status']=='PASS' for x in results.values())
