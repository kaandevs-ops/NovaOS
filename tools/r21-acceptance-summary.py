from pathlib import Path
import json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r21';local=json.loads((e/'local-results.json').read_text())['checks'];host=json.loads((e/'host-results.json').read_text())['checks'];r17=json.loads((r/'evidence/r17/results.json').read_text());r16=json.loads((r/'evidence/results.json').read_text());groups={}
for k,v in local.items():
 if k=='inherited-r17':
  for k2,v2 in r17.items():
   if k2=='r16-inherited-regressions':
    for k3,v3 in r16.items():groups['r16/'+k3]=v3
   else:groups['r17/'+k2]=v2
 else:groups['r21/'+k]=v
for k,v in host.items():groups['host/'+k]=v
assert len(groups)==37
out={'scope':'expanded serial validation groups including builds, source contracts, sanitizers, actual host process fixtures; NOT VM','PASS':sum(v['status']=='PASS' for v in groups.values()),'FAIL':sum(v['status']=='FAIL' for v in groups.values()),'PENDING':['real guest boot','DRM scanout/input latency','persistent preferences','production acceptance'],'groups':groups}
(e/'acceptance-results.json').write_text(json.dumps(out,indent=2));print(out['PASS'],'PASS',out['FAIL'],'FAIL');assert not out['FAIL']
