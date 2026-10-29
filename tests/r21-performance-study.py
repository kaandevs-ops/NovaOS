#!/usr/bin/env python3
"""Balanced sequential, CPU-affinity-pinned host study; never starts a VM."""
from pathlib import Path
import os,subprocess,json,time,fcntl,math,statistics,platform
r=Path(__file__).resolve().parents[1];out=r/'evidence/r21/performance-study';out.mkdir(exist_ok=True)
assert os.getuid()==0
lock=(r/'evidence/r21/validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
allowed=sorted(os.sched_getaffinity(0));cpu=allowed[len(allowed)//2];os.sched_setaffinity(0,{cpu})
orders=[['r19','r20','r21'],['r20','r21','r19'],['r21','r19','r20'],['r21','r20','r19'],['r19','r21','r20'],['r20','r19','r21']]
scripts={'r19':'run-r21-r19-profile.py','r20':'run-r21-r20-reference.py','r21':'run-r17-real-compositor.py'}
sources={'r19':r/'evidence/r21/r19-profile/renderer-performance.json','r20':r/'evidence/r21/before/renderer-performance.json','r21':r/'evidence/r21/renderer-performance.json'}
def optional(p):
 try:return Path(p).read_text().strip()
 except OSError:return None
def env_snapshot():return {'time_unix':time.time(),'loadavg':os.getloadavg(),'cpu_khz':optional(f'/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_cur_freq'),'thermal':{p.name:optional(p/'temp') for p in Path('/sys/class/thermal').glob('thermal_zone*')}}
runs=[]
for block,order in enumerate(orders):
 for version in order:
  name=f'{block+1:02d}-{version}';before=env_snapshot()
  with (out/(name+'.log')).open('w') as log:cp=subprocess.run(['python3','tests/'+scripts[version]],cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'NOVA_PROFILE_ONLY':'1'},timeout=120)
  if cp.returncode:raise RuntimeError(f'{name} failed: {cp.returncode}; see log')
  data=json.loads(sources[version].read_text());assert len(data['frame_times_ordered_ms'])==121
  record={'version':version,'block':block+1,'before':before,'after':env_snapshot(),'measurement':data};(out/(name+'.json')).write_text(json.dumps(record,indent=2));runs.append(record)
  print(name,'p50',data['p50_ms'],'p95',data['p95_ms'],flush=True)
def percentile(xs,p):return sorted(xs)[math.ceil(len(xs)*p)-1]
summary={}
for v in scripts:
 subset=[x['measurement'] for x in runs if x['version']==v];raw=[n for x in subset for n in x['frame_times_ordered_ms']]
 summary[v]={'runs':len(subset),'frames':len(raw),'pooled_p50_ms':percentile(raw,.5),'pooled_p95_ms':percentile(raw,.95),'pooled_max_ms':max(raw),'run_p95_range_ms':[min(x['p95_ms'] for x in subset),max(x['p95_ms'] for x in subset)],'median_run_p50_ms':statistics.median(x['p50_ms'] for x in subset),'median_run_p95_ms':statistics.median(x['p95_ms'] for x in subset),'peak_rss_kib':max(x['max_rss_kib'] for x in subset),'parts_median_p50_ms':{k:statistics.median(x['parts_p50_ms'][k] for x in subset) for k in subset[0]['parts_p50_ms']},'parts_median_p95_ms':{k:statistics.median(x['parts_p95_ms'][k] for x in subset) for k in subset[0]['parts_p95_ms']}}
report={'scope':'WSL host CPU renderer, actual apps/private fixture; NOT guest, DRM, FPS or input latency','method':'6 balanced run orders, 20 warmup +121 measured frames/run, same 1600x900 five-surface scene, no galleries during measured study, no samples discarded','affinity_cpu':cpu,'allowed_cpus_before':allowed,'kernel':platform.release(),'orders':orders,'summary':summary,'limits':['Windows background load/frequency and temperature not controlled','settings_surface component differs in geometry/function across releases; not an equal-workload speed comparison','microtimings are wall clock; scheduler noise remains','raw ordered frames retained for all 18 runs'],'runs':runs}
(out/'summary.json').write_text(json.dumps(report,indent=2));print(json.dumps(summary,indent=2))
