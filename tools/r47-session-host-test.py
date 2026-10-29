#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,tempfile,json,shutil
p=Path(__file__).resolve().parents[1];assert os.environ.get('NOVA_R47_SESSION_TEST')=='private'
session=Path(tempfile.mkdtemp(prefix='session-launch-',dir=p/'evidence/r47'));session.chmod(0o755)
runtime=session/'runtime';src=Path(json.loads((p/'evidence/r47/runtime-stage.json').read_text())['runtime'])
shutil.copytree(src,runtime,symlinks=True)
(runtime/'etc/resolv.conf').write_text(Path('/etc/resolv.conf').read_text());(runtime/'etc/resolv.conf').chmod(0o444)
ipc=Path(tempfile.mkdtemp(prefix='n47-ipc-',dir='/tmp'));ipc.chmod(0o755);root=session/'root'
# Only this namespace sees this new runtime copy remounted readonly.
subprocess.run(['mount','--bind',str(runtime),str(runtime)],check=True)
subprocess.run(['mount','-o','remount,bind,ro,nosuid,nodev',str(runtime)],check=True)
helper=session/'session';cmd=['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-static','-DNOVA_BROWSER_LAUNCHER_HOST_TEST=1',f'-DNOVA_BROWSER_RUNTIME="{runtime}"',f'-DNOVA_BROWSER_ROOT="{root}"',f'-DNOVA_BROWSER_IPC="{ipc}"',str(p/'src/compat/nova_browser_session.c'),'-o',str(helper)]
subprocess.run(cmd,check=True)
log=(session/'launch.log').open('wb')
cmd=[str(p/'build/browser-host/session-compositor-test'),str(helper),str(ipc/'client.sock'),str(session/'frame.ppm')]
if os.environ.get('NOVA_R47_TRACE')=='1':cmd=['strace','-f','-e','trace=connect,openat','-o',str(session/'trace.log'),*cmd]
r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
(session/'result.json').write_text(json.dumps({'exit':r.returncode,'scope':'host launcher/PID namespace/production compositor functions; authority fixture, NOT guest'},indent=2))
print(session,r.returncode,flush=True);raise SystemExit(r.returncode)
