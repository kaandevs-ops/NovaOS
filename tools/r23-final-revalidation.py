from pathlib import Path
import subprocess,json,fcntl
r=Path(__file__).resolve().parents[1];e=r/'evidence/r23';lock=(e/'validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
p=e/'local-results.json';d=json.loads(p.read_text());(e/'local-results-before-targeted-revalidation.json').write_text(json.dumps(d,indent=2));checks={}
for name,script in [('overview-server','r23-overview-server.sh'),('overview-client','r23-overview-client.sh'),('preferences-component','r23-preferences.sh'),('descriptor-metadata','r22-metadata.sh')]:
 with (e/('revalidated-'+name+'.log')).open('w') as f:cp=subprocess.run(['bash','tests/'+script],cwd=r,stdout=f,stderr=subprocess.STDOUT)
 result={'status':'PASS' if cp.returncode==0 else 'FAIL','exit_code':cp.returncode,'log':'revalidated-'+name+'.log','reason':'final source and expanded negative/input cases'};d['checks'][name]=result;checks[name]=result;print(name,result['status'],flush=True)
p.write_text(json.dumps(d,indent=2));(e/'targeted-revalidation.json').write_text(json.dumps(checks,indent=2));assert all(x['status']=='PASS' for x in checks.values())
