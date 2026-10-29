#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,statistics,os,fcntl,sys
r=Path(__file__).resolve().parents[1];assert os.getuid()==0;e=r/'evidence/r20';out=e/'performance-pairs';out.mkdir(exist_ok=True)
lock=(e/'validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
records=json.loads((e/'performance-paired.json').read_text())['raw'] if '--extend' in sys.argv else []
start=len(records)
for pair in range(start,start+3):
 record={}
 for version in (['r19','r20'] if pair%2==0 else ['r20','r19']):
  cmd=['python3','tests/run-r20-r19-reference.py' if version=='r19' else 'tests/run-r17-real-compositor.py']
  with (out/f'{pair+1}-{version}.log').open('w') as log:subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,check=True)
  source=e/'before/renderer-performance.json' if version=='r19' else e/'renderer-performance.json'
  data=json.loads(source.read_text());(out/f'{pair+1}-{version}.json').write_text(json.dumps(data,indent=2));record[version]=data
 records.append(record)
summary={'scope':'WSL host CPU renderer only; same canonical z-order, photo, five real surfaces and 31 timed full draws per run; NOT DRM FPS or production guest RSS','pairs':len(records),'samples_per_version':31*len(records),'raw':records,'summary':{}}
for version in ['r19','r20']:
 fields=['p50_ms','p95_ms','cpu_ms','elapsed_ms','max_rss_kib','mapped_client_bytes','snapshot_copy_p95_ms']
 summary['summary'][version]={key:{'median':statistics.median(p[version][key] for p in records),'min':min(p[version][key] for p in records),'max':max(p[version][key] for p in records)} for key in fields}
summary['status']='MEASURED';(e/'performance-paired.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary['summary'],indent=2))
