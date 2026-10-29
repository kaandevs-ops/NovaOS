from pathlib import Path
import json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r23';local=json.loads((e/'local-results.json').read_text())['checks'];host=json.loads((e/'host-results.json').read_text())['checks'];a=json.loads((r/'evidence/r17/results.json').read_text());b=json.loads((r/'evidence/results.json').read_text());groups={}
for k,v in local.items():
 if k=='inherited-r17':
  for kk,vv in a.items():
   if kk=='r16-inherited-regressions':
    for kkk,vvv in b.items():groups['r16/'+kkk]=vvv
   else:groups['r17/'+kk]=vv
 else:groups['r23/'+k]=v
for k,v in host.items():groups['host/'+k]=v
groups['host/bounded-10000-cycle-soak']=json.loads((e/'soak-result.json').read_text());assert len(groups)==43
out={'scope':'expanded serial validation groups: builds, source contracts, sanitizers and actual host fixtures; NOT VM','PASS':sum(v['status']=='PASS' for v in groups.values()),'FAIL':sum(v['status']=='FAIL' for v in groups.values()),'PENDING':['real guest boot and interaction','persistent preferences','long-duration guest soak','production acceptance'],'groups':groups}
(e/'acceptance-results.json').write_text(json.dumps(out,indent=2));print(out['PASS'],'PASS',out['FAIL'],'FAIL');assert not out['FAIL']
