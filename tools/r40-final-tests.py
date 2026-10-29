from pathlib import Path
import subprocess,json,hashlib
r=Path(__file__).resolve().parents[1];e=r/'evidence/r40';results={}
shared=['files/src/nova_files_core.c','files/src/nova_files_ui.c','files/src/nova_files_font.c']
steps=[('native-x86',['bash','scripts/build-r16-files.sh','x86_64']),('native-arm',['bash','scripts/build-r16-files.sh','arm64']),('pages-asan-build',['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/runtime','-Ifiles/src','tests/r40-files-pages.c',*shared,'-o','build/x86_64/r40-files/test-pages']),('pages-asan-run',['build/x86_64/r40-files/test-pages','evidence/r40/files-page.ppm','evidence/r40/files-query.ppm']),('preview-asan-build',['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/runtime','-Ifiles/src','tests/r38-preview-ui.c',*shared,'-o','build/x86_64/r40-files/test-preview']),('preview-asan-run',['build/x86_64/r40-files/test-preview']),('process-v6-mock',['python3','tests/r40-files-process.py'])]
for name,cmd in steps:
 with (e/(name+'.log')).open('w') as log:p=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT)
 results[name]={'status':'PASS' if not p.returncode else 'FAIL','exit':p.returncode};(e/'final-build-tests.json').write_text(json.dumps(results,indent=2));print(name,results[name],flush=True)
 if p.returncode:raise SystemExit(p.returncode)
artifacts={}
for a in ('x86_64','arm64'):
 f=r/f'build/{a}/r16-files/nova-files';artifacts[a]={'sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'file':subprocess.check_output(['file',str(f)],text=True).strip(),'guest':'PENDING'}
(e/'binary-artifacts.json').write_text(json.dumps(artifacts,indent=2))
