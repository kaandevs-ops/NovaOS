from pathlib import Path
import subprocess,sys,shutil,json
r=Path(__file__).resolve().parents[1];arch=sys.argv[1] if len(sys.argv)>1 else 'x86_64'
if arch!='x86_64':raise SystemExit('R59 store runtime currently validated for x86_64 only; do not enable on ARM64')
out=r/'build/x86_64/r59-store';out.mkdir(parents=True,exist_ok=True);helper=out/'nova-store-session';header=r/'build/x86_64/r33-catalog/catalog.h'
cmd=['gcc','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_PACKAGE_CATALOG="{header}"','src/software/nova_store_session.c','src/software/nova_repository.c','-lcrypto','-ldl','-pthread','-o',str(helper)]
with (out/'build.log').open('w') as log:subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,check=True)
# A fresh staging directory, never reuse a prior signed runtime.
import tempfile
parent=Path(tempfile.mkdtemp(prefix='runtime-',dir=out));runtime=parent/'Software';subprocess.run([sys.executable,str(r/'tools/r59-stage-store-runtime.py'),str(runtime)],check=True)
root=r/'build/x86_64/rootfs';dest=root/'System/Software'
if dest.exists():raise SystemExit('Refusing to replace an existing store runtime; use fresh userland staging')
shutil.copytree(runtime,dest,symlinks=True);
import os
os.chown(dest/'home/nova',12004,12004);os.chown(dest/'home/nova/.nova-apps',12004,12004)
shutil.copyfile(helper,root/'System/Core/nova-store-session');(root/'System/Core/nova-store-session').chmod(0o555)
print(json.dumps({'status':'BUILD PASS','runtime_configured':False,'repository':'no default signing key/server; Internet UI remains unavailable','guest':'PENDING'}))
