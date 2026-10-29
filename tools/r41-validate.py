from pathlib import Path
import subprocess,json,hashlib,os
r=Path(__file__).resolve().parents[1];e=r/'evidence/r41/final';e.mkdir(exist_ok=True);(r/'evidence/r40').mkdir(exist_ok=True);(r/'build/x86_64/r40-files').mkdir(parents=True,exist_ok=True)
shared=['files/src/nova_files_core.c','files/src/nova_files_ui.c','files/src/nova_files_font.c'];results={}
steps=[('native-x86',['bash','scripts/build-r17-native.sh','x86_64']),('native-arm',['bash','scripts/build-r17-native.sh','arm64']),('unsigned-rootfs-x86',['python3','scripts/build-r17-unsigned-rootfs.py','x86_64']),('unsigned-rootfs-arm',['python3','scripts/build-r17-unsigned-rootfs.py','arm64']),('posix-worker',['python3','tools/r41-posix-worker-test.py']),('legacy-worker',['python3','tools/r41-legacy-worker-test.py']),('legacy-client',['python3','tools/r41-legacy-client-test.py']),('R38-lab-security',['python3','tools/r38-lab-security-test.py']),('posix-ui-asan-ubsan',['python3','tools/r41-posix-ui-test.py']),('details-replies-asan-build',['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-Isrc/runtime','-Ifiles/src','tests/r41-details-replies.c','src/runtime/nova_runtime.c','-o','evidence/r41/test-details-replies']),('details-replies-14-negative',['evidence/r41/test-details-replies']),('R40-pages-asan-build',['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-Isrc/runtime','-Ifiles/src','tests/r40-files-pages.c',*shared,'-o','build/x86_64/r40-files/test-pages']),('R40-pages-asan-run',['build/x86_64/r40-files/test-pages','evidence/r41/files-page.ppm','evidence/r41/files-query.ppm']),('R40-process-v6-mock-peer',['python3','tests/r40-files-process.py']),('R38-preview-security',['python3','tools/r38-home-preview-test.py']),('R40-rehearsal-43',['python3','tools/r40-rehearsal-reboot-test.py']),('R20-navigation',['bash','tests/r20-navigation.sh']),('R21-files-view',['bash','tests/r21-files-view.sh']),('R22-metadata',['bash','tests/r22-metadata.sh'])]
for name,cmd in steps:
 print('START',name,flush=True)
 with (e/(name+'.log')).open('w') as log:p=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'NOVA_R38_WEEK4_LAB':'1'})
 results[name]={'status':'PASS' if not p.returncode else 'FAIL','exit':p.returncode};(e/'validation-results.json').write_text(json.dumps(results,indent=2));print(name,results[name],flush=True)
 if p.returncode:raise SystemExit(p.returncode)
artifacts={}
for arch in ('x86_64','arm64'):
 artifacts[arch]={}
 for part in ('init','System/Apps/nova-files'):
  f=r/'build'/arch/'r17-native'/part;artifacts[arch][part]={'sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'file':subprocess.check_output(['file',str(f)],text=True).strip(),'guest':'PENDING'}
(e/'binary-artifacts.json').write_text(json.dumps(artifacts,indent=2))
