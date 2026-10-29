from pathlib import Path
import subprocess,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r41';old=r.parent/'phase19-desktop-r40-files-navigation-candidate';results={}
for label,root in [('R40',old),('R41',r)]:
 exe=e/(label+'-posix-ui');cmd=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root/'files/src'),'-I'+str(root/'src/runtime'),*(['-DNOVA_R41_POSIX'] if label=='R41' else []),str(r/'tests/r41-posix-ui.c'),str(root/'files/src/nova_files_core.c'),str(root/'files/src/nova_files_font.c'),str(root/'src/runtime/nova_runtime.c'),str(root/'src/runtime/nova_font.c'),'-o',str(exe)]
 with (e/(label+'-posix-ui-build.log')).open('w') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
 with (e/(label+'-posix-ui-run.log')).open('w') as log:subprocess.run([str(exe),*[str(e/(label+'-'+n+'.ppm')) for n in ('mode600','mode000','owner12006')]],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=60)
 results[label]='PASS'
(e/'posix-ui-result.json').write_text(json.dumps({'status':'PASS','scope':'native renderer actual host statx and permission/owner changes; ASAN+UBSAN; NOT VM','results':results},indent=2));print(results)
