from pathlib import Path
import json,subprocess,statistics,fcntl
r=Path(__file__).resolve().parents[1];e=r/'evidence/r20';old=r.parent/'phase19-desktop-r19-premium-design-candidate';lock=(e/'validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
for version,source,root in [('r19','r20-r19-render-cost.c',old),('r20','r20-render-cost.c',r)]:
 cmd=['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-I'+str(root/'src/runtime'),str(r/'tests'/source),str(root/'src/runtime/nova_runtime.c'),str(root/'src/runtime/nova_font.c'),'-Wl,--gc-sections','-o',str(r/'build/x86_64/r17-native'/('cost-'+version))];subprocess.run(cmd,check=True)
records=[]
for n in range(3):
 row={}
 for version in (['r19','r20'] if n%2==0 else ['r20','r19']):row[version]=json.loads(subprocess.check_output([str(r/'build/x86_64/r17-native'/('cost-'+version))],text=True))
 records.append(row)
summary={version:{key:statistics.median(row[version][key] for row in records) for key in ['text_26chars_us','app_icon_48_us','cached_shadow_800x500_us']} for version in ['r19','r20']}
(e/'component-cost-paired.json').write_text(json.dumps({'scope':'3 paired warm O2 CPU microbenchmarks, same sample string/app icon/shadow; no runtime rasterizer or blur; NOT DRM frame time','raw':records,'median':summary},indent=2));print(json.dumps(summary,indent=2))
