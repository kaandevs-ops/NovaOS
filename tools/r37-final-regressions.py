from pathlib import Path
import subprocess,json,sys
r=Path(__file__).resolve().parents[1];e=r/'evidence/r37';result={}
steps=[('network-unit',[str(e/'network-unit')]),('network-policy',[str(e/'network-policy')])]+[(n,[sys.executable,str(r/'tools'/f)]) for n,f in [('network-process','r37-network-process-test.py'),('network-authority','r37-network-auth-test.py'),('dynamic-packages','r36-dynamic-runtime-test.py'),('runtime-authority','r36-runtime-authority-test.py'),('readonly-files-worker','r26-home-worker-test.py')]]
for n,cmd in steps:
 with (e/(n+'-final.log')).open('w') as log:cp=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 result[n]={'status':'PASS' if cp.returncode==0 else 'FAIL','exit_code':cp.returncode};print(n,result[n],flush=True)
 (e/'final-regressions.json').write_text(json.dumps(result,indent=2))
 if cp.returncode:raise SystemExit(cp.returncode)
