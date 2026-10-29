from pathlib import Path
import subprocess,json,os
r=Path(__file__).resolve().parents[1];e=r/'evidence/r25/regression';e.mkdir(parents=True,exist_ok=True)
for name in ['evidence/r21','evidence/continuation','build/tmp','build/x86_64/r16-integration']:(r/name).mkdir(parents=True,exist_ok=True)
checks=[('sandbox-fixture-build',['bash','scripts/build-r16-integration.sh','x86_64']),('r24-units',['bash','tests/r24-units.sh']),('telemetry-bound',['python3','tests/r24-telemetry-budget.py']),('authority-wakeup',['python3','tests/r24-authority-wakeup.py'])]
for name in ['r23-overview-server','r23-overview-client','r23-preferences','r22-quick','r22-metadata','r21-blit-equivalence','r21-theme','r21-files-view','r20-navigation','r18-visual-ui','r18-shadow-equivalence','r19-native-design','r19-compositor-design']:checks.append((name,['bash','tests/'+name+'.sh']))
checks += [('three-theme-contrast',['python3','tests/r19-contrast.py']),('principal-runtime',['bash','tests/r16-compositor-runtime.sh']),('principal-source',['bash','tests/stage5a-phase16-peer-principal-source.sh']),('catalog-grant',['runuser','-u','pc','--','bash','tests/r16-trusted-runtime.sh']),('r13-comparison',['python3','tests/r16-security-invariants.py']),('authority-identity',['python3','tools/r26-security-audit.py']),('kernel-sandbox',['python3','tests/r16-sandbox-runtime.py'])]
result={}
for name,cmd in checks:
 print('START',name,flush=True)
 with (e/(name+'.log')).open('w') as log:
  p=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
 result[name]={'status':'PASS' if p.returncode==0 else 'FAIL','exit_code':p.returncode}
 (e/'results.json').write_text(json.dumps(result,indent=2));print(name,result[name],flush=True)
 # Preserve every failure and keep running independent tests.
print('PASS',sum(x['status']=='PASS' for x in result.values()),'FAIL',sum(x['status']=='FAIL' for x in result.values()))
raise SystemExit(any(x['status']=='FAIL' for x in result.values()))
