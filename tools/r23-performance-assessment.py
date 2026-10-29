from pathlib import Path
import json,random,statistics
r=Path(__file__).resolve().parents[1];e=r/'evidence/r23/performance-study';data=json.loads((e/'summary.json').read_text());rng=random.Random(22);out={}
for a,b in [('r21','r22'),('r22','r23')]:
 changes={}
 for metric in ['p50_ms','p95_ms']:
  rows=[]
  for block in range(1,7):
   x=next(x['measurement'] for x in data['runs'] if x['block']==block and x['version']==a);y=next(x['measurement'] for x in data['runs'] if x['block']==block and x['version']==b);rows.append((y[metric]/x[metric]-1)*100)
  sims=sorted(statistics.mean(rng.choices(rows,k=6)) for _ in range(10000));changes[metric]={'block_percent_changes':rows,'mean_block_percent_change':statistics.mean(rows),'bootstrap_mean_95_interval_percent':[sims[249],sims[9749]]}
 out[a+'-to-'+b]=changes
(e/'paired-uncertainty.json').write_text(json.dumps({'method':'six balanced blocks; 10000 deterministic block bootstrap replicates, no discarded timings; small sample and host frequency uncontrolled; not proof of causality','comparisons':out},indent=2));print(json.dumps(out,indent=2))
