from pathlib import Path
import subprocess,json,statistics
r=Path(__file__).resolve().parents[1];out=r/'evidence/r54/performance';out.mkdir(parents=True,exist_ok=True)
old=r.parent/'phase19-desktop-r53-dock-comfort-candidate/src/services/compositor.c'
for name,source,extra in [('R53',old,[]),('R54',r/'src/services/compositor.c',['-DNOVA_BENCH_R54=1'])]:
 cmd=['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-ffunction-sections','-fdata-sections','-I'+str(source.parents[1]/'runtime'),'-DNOVA_BENCH_COMPOSITOR="'+str(source)+'"',*extra,'tests/r54-render-cost.c',str(source.parents[1]/'runtime/nova_runtime.c'),str(source.parents[1]/'runtime/nova_font.c'),'-Wl,--gc-sections','-o',str(out/name)]
 subprocess.run(cmd,cwd=r,check=True)
raw=[]
for w,h in [(1280,800),(1600,900)]:
 for block in range(5):
  for name in (['R53','R54'] if block%2==0 else ['R54','R53']):
   result=json.loads(subprocess.check_output([str(out/name),str(w),str(h),'0'],text=True));raw.append(dict(version=name,width=w,height=h,block=block,mode='same two desktop entries',**result))
for mode in [1,2]:
 result=json.loads(subprocess.check_output([str(out/'R54'),'1600','900',str(mode)],text=True));raw.append(dict(version='R54',width=1600,height=900,block=0,mode='text preview' if mode==1 else 'trash eight rows',**result))
def pct(v,p):return sorted(v)[min(len(v)-1,int((len(v)-1)*p))]/1e6
summary=[]
for w,h in [(1280,800),(1600,900)]:
 for name in ['R53','R54']:
  rows=[x for x in raw if x['version']==name and x['width']==w and x['mode']=='same two desktop entries'];samples=[v for x in rows for v in x['ns']]
  summary.append(dict(version=name,width=w,height=h,p50_ms=pct(samples,.5),p95_ms=pct(samples,.95),peak_rss_kib=max(x['peak_rss_kib'] for x in rows),samples=len(samples),blocks=len(rows)))
for row in raw:
 if row['mode']!='same two desktop entries':summary.append(dict(version='R54',mode=row['mode'],p50_ms=pct(row['ns'],.5),p95_ms=pct(row['ns'],.95),peak_rss_kib=row['peak_rss_kib']))
result=dict(scope='HOST FULL-SCENE CPU RENDER FIXTURE ONLY; NOT FPS, DRM PRESENT OR INPUT LATENCY',method='same compiler/options/host, 5 interleaved blocks, 5 warmup + 60 measured frames per block',summary=summary,raw=raw)
(out/'paired-render.json').write_text(json.dumps(result,indent=2));print(json.dumps(summary,indent=2))
