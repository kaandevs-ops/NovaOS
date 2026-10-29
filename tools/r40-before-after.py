from pathlib import Path
import os,subprocess,json,shutil
r=Path(__file__).resolve().parents[1];old=r.parent/'phase19-desktop-r39-readiness-candidate';e=r/'evidence/r40';e.mkdir(parents=True,exist_ok=True)
prior=json.loads((e/'before-after.json').read_text()) if (e/'before-after.json').exists() else {}
fixture=Path(prior.get('directory','/nonexistent-r40-baseline'))
if not fixture.is_dir():
 import tempfile
 fixture=Path(tempfile.mkdtemp(prefix='nova-r40-compare-'))
 for i in range(202):(fixture/f'Document-{i:03d}.txt').write_text(f'Real file {i}\n')
ordered=[x.name for x in os.scandir(fixture)];query=prior.get('query',ordered[150]);assert query not in ordered[:48]
(r/'build/x86_64/r40-files').mkdir(parents=True,exist_ok=True)
results={'scope':'same real host directory, same query and native Files renderer; not a VM; list timing is not FPS','directory':str(fixture),'query':query,'runs':{}}
for name,root,expected in [('R39',old,0),('R40',r,1)]:
 binary=r/f'build/x86_64/r40-files/compare-{name}'
 cmd=['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'files/src'),'-I'+str(root/'src/runtime'),str(r/'tests/r40-before-after.c'),str(root/'files/src/nova_files_core.c'),str(root/'files/src/nova_files_ui.c'),str(root/'files/src/nova_files_font.c'),'-o',str(binary)]
 subprocess.run(cmd,check=True)
 text=subprocess.check_output([str(binary),str(fixture),query,str(expected),str(e/f'{name}-query.ppm')],text=True)
 results['runs'][name]=json.loads(text);print(name,text.strip(),flush=True)
if (e/'before-after.json').exists() and not (e/'before-after-initial.json').exists():shutil.copy2(e/'before-after.json',e/'before-after-initial.json')
(e/'before-after.json').write_text(json.dumps(results,indent=2));print('R39_BLIND_SPOT_REPRODUCED_R40_REAL_FILE_FOUND')