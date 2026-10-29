#!/usr/bin/env python3
"""Serial R20 validation. Frozen suites move build/ temporarily; one workspace lock prevents overlap."""
from pathlib import Path
import subprocess,json,os,fcntl,sys,time
r=Path(__file__).resolve().parents[1];assert r.name=='phase19-desktop-r24-performance-evolution-candidate';out=r/'evidence/r24';out.mkdir(exist_ok=True)
lock=(out/'validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
(r/'build/x86_64/r17-native').mkdir(parents=True,exist_ok=True)
(r/'evidence/continuation').mkdir(parents=True,exist_ok=True)
for version in range(16,24):(r/'evidence'/('r'+str(version))).mkdir(parents=True,exist_ok=True)
host='--host-fixtures' in sys.argv
if host:assert os.getuid()==0
checks=({'kernel-sandbox':['python3','tests/r16-sandbox-runtime.py'],'r23-real-reference':['python3','tests/run-r24-r23-reference.py'],'legacy-clients-new-server':['python3','tests/run-r21-legacy-clients.py'],'r24-real-compositor':['python3','tests/run-r17-real-compositor.py']} if host else {
 'r24-input-damage-properties-diagnostics':['bash','tests/r24-units.sh'],
 'overview-server':['bash','tests/r23-overview-server.sh'],
 'overview-client':['bash','tests/r23-overview-client.sh'],
 'preferences-component':['bash','tests/r23-preferences.sh'],
 'quick-settings':['bash','tests/r22-quick.sh'],
 'descriptor-metadata':['bash','tests/r22-metadata.sh'],
 'opaque-span-blit':['bash','tests/r21-blit-equivalence.sh'],
 'theme-settings-security':['bash','tests/r21-theme.sh'],
 'files-filter-sort':['bash','tests/r21-files-view.sh'],
 'navigation-security':['bash','tests/r20-navigation.sh'],
 'inherited-r17':['python3','tests/run-r17.py'],
 'updated-visual-ui':['bash','tests/r18-visual-ui.sh'],
 'smooth-shadow-equivalence':['bash','tests/r18-shadow-equivalence.sh'],
 'three-theme-contrast':['python3','tests/r19-contrast.py'],
 'native-design-sanitizers':['bash','tests/r19-native-design.sh'],
 'compositor-design-sanitizers':['bash','tests/r19-compositor-design.sh'],
 'r20-reference-harness-build':['bash','tests/build-r24-references.sh']})
results={};started=time.time()
for name,cmd in checks.items():
 with (out/('final-'+name+'.log')).open('w') as log:cp=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
 results[name]={'status':'PASS' if not cp.returncode else 'FAIL','exit_code':cp.returncode};print(name,results[name]['status'],flush=True)
(out/('host-results.json' if host else 'local-results.json')).write_text(json.dumps({'checks':results,'started_unix':started,'ended_unix':time.time(),'scope':'private host fixtures, NOT VM' if host else 'source/build/runtime/sanitizer checks'},indent=2))
raise SystemExit(any(v['exit_code'] for v in results.values()))
