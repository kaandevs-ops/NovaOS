#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,statistics,os
r=Path(__file__).resolve().parents[1];assert os.getuid()==0;e=r/'evidence/r18';out=e/'performance-pairs';out.mkdir(exist_ok=True)
records=[]
for pair in range(3):
 record={}
 for version in (['r17','r18'] if pair%2==0 else ['r18','r17']):
  cmd=['python3','tests/run-r18-r17-reference.py' if version=='r17' else 'tests/run-r17-real-compositor.py']
  with (out/f'{pair+1}-{version}.log').open('w') as log:subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,check=True)
  source=e/'before/renderer-performance.json' if version=='r17' else r/'evidence/r17/renderer-performance.json'
  data=json.loads(source.read_text());(out/f'{pair+1}-{version}.json').write_text(json.dumps(data,indent=2));record[version]=data
 records.append(record)
summary={'scope':'WSL host CPU renderer only; same canonical z-order, photo, five real surfaces and 31 timed full draws per run; NOT DRM FPS or production guest RSS','pairs':3,'samples_per_version':93,'raw':records,'summary':{}}
for version in ['r17','r18']:
 fields=['p50_ms','p95_ms','cpu_ms','elapsed_ms','max_rss_kib','mapped_client_bytes','snapshot_copy_p95_ms']
 summary['summary'][version]={key:{'median':statistics.median(p[version][key] for p in records),'min':min(p[version][key] for p in records),'max':max(p[version][key] for p in records)} for key in fields}
summary['status']='MEASURED';(e/'performance-paired.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary['summary'],indent=2))
