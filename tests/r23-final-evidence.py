from pathlib import Path
import subprocess,time,json,os
r=Path(__file__).resolve().parents[1];assert os.getuid()==0;e=r/'evidence/r23'
steps=[('targeted-revalidation','tools/r23-final-revalidation.py',[]),('host-final','tests/run-r23.py',['--host-fixtures']),('native-verification','tools/r23-verify-native.py',[]),('source-security','tools/r23-security-source-audit.py',[]),('guest-preflight','tools/r23-guest-preflight.py',[]),('feature-cost','tests/r23-feature-cost.py',[]),('soak','tests/run-r23-soak.py',[]),('performance','tests/r23-performance-study.py',[]),('uncertainty','tools/r23-performance-assessment.py',[]),('gallery','tools/r23-export-gallery.py',[]),('acceptance','tools/r23-acceptance-summary.py',[])]
results={}
for name,script,args in steps:
 print('START',name,flush=True);start=time.time()
 with (e/('evidence-'+name+'.log')).open('w') as f:cp=subprocess.run(['python3',script,*args],cwd=r,stdout=f,stderr=subprocess.STDOUT)
 results[name]={'status':'PASS' if cp.returncode==0 else 'FAIL','exit_code':cp.returncode,'seconds':time.time()-start};(e/'final-evidence-results.json').write_text(json.dumps(results,indent=2));print(name,results[name],flush=True)
 if cp.returncode:raise SystemExit(cp.returncode)
