#!/usr/bin/env python3
"""Host private-mount loader probe, not guest boot or sandbox acceptance."""
import json,os,subprocess
from pathlib import Path
p=Path(__file__).resolve().parents[1]
if os.environ.get('NOVA_R47_PROBE')!='private':raise SystemExit('run only with unshare --mount --fork --propagation private')
root=Path(json.loads((p/'evidence/r47/runtime-stage.json').read_text())['runtime'])
subprocess.run(['mount','-t','proc','-o','nosuid,nodev,noexec','proc',str(root/'proc')],check=True)
results=[]
try:
 for binary,args in [('/opt/firefox/firefox',['--version']),('/usr/bin/Xvfb',['-help'])]:
  r=subprocess.run(['/usr/sbin/chroot','--userspec=1000:1000',str(root),binary,*args],capture_output=True,text=True,timeout=20)
  results.append({'binary':binary,'exit':r.returncode,'stdout':r.stdout[:3000],'stderr':r.stderr[:3000]})
finally:subprocess.run(['umount',str(root/'proc')],check=True)
(p/'evidence/r47/runtime-private-probes.json').write_text(json.dumps(results,indent=2))
print(json.dumps(results,indent=2));raise SystemExit(any(r['exit'] for r in results))
