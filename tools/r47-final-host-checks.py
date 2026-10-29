#!/usr/bin/env python3
from pathlib import Path
import os,json,subprocess,hashlib
p=Path(__file__).resolve().parents[1];e=p/'evidence/r47';results={}
def run(name,cmd,env=None):
 with (e/(name+'.log')).open('w') as log:r=subprocess.run(cmd,cwd=p,env=env,stdout=log,stderr=subprocess.STDOUT)
 results[name]={'exit':r.returncode,'status':'PASS' if not r.returncode else 'FAIL'};(e/'final-host-checks.json').write_text(json.dumps(results,indent=2));print(name,results[name],flush=True)
 if r.returncode:raise SystemExit(r.returncode)
run('shell-syntax',['bash','-n','scripts/build-userland.sh','scripts/build-initramfs.sh','scripts/build-disk.sh','scripts/verity/build-system-slot.sh'])
run('novad-x86-enabled-syntax',['clang','-std=c11','-Wall','-Wextra','-Werror','-DNOVA_R47_BROWSER_COMPAT=1','-Isrc/runtime','-fsyntax-only','src/init/main.c'])
run('novad-arm-legacy-syntax',['clang','--target=aarch64-linux-gnu','-std=c11','-Wall','-Wextra','-Werror','-Isrc/runtime','-fsyntax-only','src/init/main.c'])
common=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined','-Isrc/runtime']
for mode in [0,1]:
 binary=p/('build/browser-host/catalog-final-'+str(mode))
 run('catalog-build-'+str(mode),[*common,'-DNOVA_R47_BROWSER_COMPAT='+str(mode),'tests/r47-browser-catalog.c','src/runtime/nova_runtime.c','src/runtime/nova_sha256.c','-Wl,--gc-sections','-o',str(binary)])
 run('catalog-run-'+str(mode),[str(binary)])
run('legacy-compositor-regression',['bash','tests/r16-compositor-runtime.sh'])
run('pixel-equivalence',[str(p/'build/browser-host/pixel-test')])
run('native-bridge-build',['bash','tools/r47-build-native-bridge.sh'])
run('runtime-stage',['python3','tools/r47-stage-browser-runtime.py'])
env={**os.environ,'NOVA_R47_BROWSER_COMPAT':'1','NOVA_R47_BROWSER_TEST_NAT':'1'}
run('unsigned-root-staging',['python3','tools/r47-install-browser-runtime.py',str(p/'build/x86_64/rootfs')],env)
run('unsigned-manifest-generation',['python3','scripts/generate-integrity-manifest.py',str(p/'build/x86_64/rootfs')])
manifest=p/'build/x86_64/rootfs/System/Integrity/manifest.sha256';assert manifest.stat().st_size<131072
staged=p/'build/x86_64/rootfs';runtime=staged/'System/Browser'
files=sum(f.stat().st_size for f in runtime.rglob('*') if f.is_file());assert files+40*1024*1024<512*1024*1024
listing=subprocess.check_output(['find','.', '-path','./System/Browser','-prune','-o','-print'],cwd=staged,text=True);assert './System/Browser/' not in listing
(e/'guest-source-preflight.json').write_text(json.dumps({'browser_bytes':files,'current_partial_manifest_bytes':manifest.stat().st_size,'integrity_buffer':131072,'system_data_mib':512,'system_slot_mib':520,'runtime_excluded_from_initramfs':True,'signing_performed':False,'new_vm_created':False,'guest':'PENDING'},indent=2))
run('session-fixture-build',[*common,'-Wno-misleading-indentation','tests/r47-browser-session-compositor.c','src/runtime/nova_runtime.c','src/runtime/nova_font.c','-Wl,--gc-sections','-o',str(p/'build/browser-host/session-compositor-test')])
run('full-private-session',['unshare','--mount','--fork','--propagation','private','python3','tools/r47-session-host-test.py'],{**os.environ,'NOVA_R47_SESSION_TEST':'private'})
old=p.parent/'phase19-desktop-r46-native-apps-candidate';before=json.loads((e/'r46-source-before.json').read_text());diff=[name for name,h in before.items() if not (old/name).is_file() or hashlib.sha256((old/name).read_bytes()).hexdigest()!=h];assert not diff
(e/'final-source-protection.json').write_text(json.dumps({'checked':len(before),'differences':diff,'scope':'R46 source records only, not live VM disks'},indent=2))
