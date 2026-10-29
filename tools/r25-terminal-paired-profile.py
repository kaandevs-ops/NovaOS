from pathlib import Path
import subprocess,json,tempfile,statistics,hashlib
r=Path(__file__).resolve().parents[1];e=r/'evidence/r25/terminal-paired';e.mkdir(parents=True,exist_ok=True)
old=r/'evidence/r25/desktop-vmwlhmgb/desktop';new=r/'evidence/r25/desktop-uvcyc8_w/desktop'
records=[];paths={}
for round in range(3):
 for tag,binary in ([('before',old),('after',new)] if round%2==0 else [('after',new),('before',old)]):
  out=Path(tempfile.mkdtemp(prefix=f'{round}-{tag}-',dir=e))
  with (out/'run.log').open('w') as log:p=subprocess.run([str(binary),str(out)],cwd=r,stdout=log,stderr=subprocess.STDOUT,timeout=90)
  assert not p.returncode,out
  perf=json.loads((out/'terminal-render.json').read_text());records.append({'round':round,'tag':tag,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),**perf});paths[(round,tag)]=out;print(round,tag,perf['p50_ms'],perf['p95_ms'],flush=True)
 for a in paths[(round,'before')].glob('*.ppm'):
  b=paths[(round,'after')]/a.name;assert a.read_bytes()==b.read_bytes(),a.name
summary={tag:{key:statistics.median(x[key] for x in records if x['tag']==tag) for key in ['p50_ms','p95_ms','peak_rss_kib']} for tag in ['before','after']}
(e/'results.json').write_text(json.dumps({'scope':'three alternating pairs, same WSL host/native workload/real shell output, 1600x900 20 warmup+121 frames; VM idle, frequency/background load not controlled; not screen FPS','pixel_identical_frames':36,'summary':summary,'runs':records},indent=2));print(summary)
