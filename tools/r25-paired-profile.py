from pathlib import Path
import os,json,subprocess,statistics,hashlib
r=Path(__file__).resolve().parents[1];e=r/'evidence/r25/paired-profile-final';e.mkdir(parents=True,exist_ok=True)
bins={'R24':r.parent/'phase19-desktop-r24-performance-evolution-candidate/build/x86_64/r17-native/test-real-compositor','R25':r/'build/x86_64/r17-native/test-real-compositor'}
records=[]
for iteration in range(3):
 for tag in (['R24','R25'] if iteration%2==0 else ['R25','R24']):
  d=e/f'{iteration}-{tag}';d.mkdir(exist_ok=True)
  env={**os.environ,'NOVA_PROFILE_ONLY':'1','NOVA_R24_EVIDENCE_DIR':str(d),'NOVA_R24_COMPOSITOR_TEST_BINARY':str(bins[tag])}
  env.pop('NOVA_R25_TERMINAL_TEST',None)
  with (d/'runner.log').open('w') as log:p=subprocess.run(['python3','tests/run-r17-real-compositor.py'],cwd=r,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=90)
  assert p.returncode==0,(tag,d)
  perf=json.loads((d/'renderer-performance.json').read_text());records.append({'round':iteration,'tag':tag,'binary_sha256':hashlib.sha256(bins[tag].read_bytes()).hexdigest(),**perf});print(iteration,tag,perf['p50_ms'],perf['p95_ms'],flush=True)
summary={tag:{key:statistics.median(x[key] for x in records if x['tag']==tag) for key in ['p50_ms','p95_ms','max_rss_kib']} for tag in bins}
(e/'results.json').write_text(json.dumps({'scope':'same WSL host, 1600x900, five real surfaces, 20 warmup/121 samples, terminal hidden, three alternating pairs; authority fixture and no DRM/VM; frequency/background load uncontrolled','summary':summary,'runs':records},indent=2));print(summary)
