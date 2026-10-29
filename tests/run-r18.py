#!/usr/bin/env python3
"""Sequential execution: frozen suites temporarily move build/, so do not run siblings concurrently."""
from pathlib import Path
import subprocess,json,sys,os
r=Path(__file__).resolve().parents[1];assert r.name=='phase19-desktop-r18-visual-excellence-candidate'
out=r/'evidence/r18';out.mkdir(exist_ok=True)
checks=({'kernel-sandbox':['python3','tests/r16-sandbox-runtime.py'],'real-compositor-three-apps':['python3','tests/run-r17-real-compositor.py']}
 if '--privileged-host' in sys.argv else {'inherited-r17':['python3','tests/run-r17.py'],'visual-ui-sanitizers':['bash','tests/r18-visual-ui.sh'],'shadow-equivalence':['bash','tests/r18-shadow-equivalence.sh'],'text-contrast':['python3','tests/r18-contrast.py']})
if '--privileged-host' in sys.argv:assert os.getuid()==0
results={}
for name,cmd in checks.items():
 with (out/(name+'.log')).open('w') as log:cp=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 results[name]={'status':'PASS' if not cp.returncode else 'FAIL','exit_code':cp.returncode};print(name,results[name],flush=True)
(out/('privileged-host-results.json' if '--privileged-host' in sys.argv else 'local-results.json')).write_text(json.dumps(results,indent=2))
raise SystemExit(any(x['exit_code'] for x in results.values()))
