from pathlib import Path
import subprocess,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r40';results={}
steps=[('R39-course-42',['python3','tools/r39-readiness-process-test.py']),('R38-lab-security',['python3','tools/r38-lab-security-test.py']),('R38-preview-worker',['python3','tools/r38-home-preview-test.py']),('R20-navigation',['bash','tests/r20-navigation.sh']),('R21-view',['bash','tests/r21-files-view.sh']),('R40-process-v6-mock-peer',['python3','tests/r40-files-process.py'])]
for name,cmd in steps:
 with (e/(name+'.log')).open('w') as log:p=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 results[name]={'status':'PASS' if not p.returncode else 'FAIL','exit':p.returncode};(e/'regressions.json').write_text(json.dumps(results,indent=2));print(name,results[name],flush=True)
 if p.returncode:raise SystemExit(p.returncode)
