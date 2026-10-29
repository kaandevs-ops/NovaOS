#!/usr/bin/env python3
"""Serial R19 validation. Frozen suites move build/ temporarily; one workspace lock prevents overlap."""
from pathlib import Path
import subprocess,json,os,fcntl,sys,time
r=Path(__file__).resolve().parents[1];assert r.name=='phase19-desktop-r19-premium-design-candidate';out=r/'evidence/r19';out.mkdir(exist_ok=True)
lock=(out/'validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
host='--host-fixtures' in sys.argv
if host:assert os.getuid()==0
checks=({'kernel-sandbox':['python3','tests/r16-sandbox-runtime.py'],'r18-real-reference':['python3','tests/run-r19-r18-reference.py'],'r19-real-compositor':['python3','tests/run-r17-real-compositor.py']} if host else {
 'inherited-r17':['python3','tests/run-r17.py'],
 'updated-visual-ui':['bash','tests/r18-visual-ui.sh'],
 'smooth-shadow-equivalence':['bash','tests/r18-shadow-equivalence.sh'],
 'three-theme-contrast':['python3','tests/r19-contrast.py'],
 'native-design-sanitizers':['bash','tests/r19-native-design.sh'],
 'compositor-design-sanitizers':['bash','tests/r19-compositor-design.sh'],
 'r18-reference-harness-build':['bash','tests/build-r19-r18-reference.sh']})
results={};started=time.time()
for name,cmd in checks.items():
 with (out/('final-'+name+'.log')).open('w') as log:cp=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
 results[name]={'status':'PASS' if not cp.returncode else 'FAIL','exit_code':cp.returncode};print(name,results[name]['status'],flush=True)
(out/('host-results.json' if host else 'local-results.json')).write_text(json.dumps({'checks':results,'started_unix':started,'ended_unix':time.time(),'scope':'private host fixtures, NOT VM' if host else 'source/build/runtime/sanitizer checks'},indent=2))
raise SystemExit(any(v['exit_code'] for v in results.values()))
