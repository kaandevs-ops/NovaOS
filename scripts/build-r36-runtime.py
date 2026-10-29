"""Build a bounded read-only glibc runtime and a real dynamic Lua executable.
Libraries come from the installed architecture toolchain, never from a client.
No network, package repository or new syscall grant is introduced.
"""
from pathlib import Path
import subprocess,sys,hashlib,json,shutil,struct,os
r=Path(__file__).resolve().parents[1];arch=sys.argv[1];assert arch in ('x86_64','arm64')
cc='gcc' if arch=='x86_64' else 'aarch64-linux-gnu-gcc';machine=62 if arch=='x86_64' else 183
bundle=r/f'build/{arch}/r25-terminal/System/Terminal';runtime=bundle/'Runtime'
for name in ['lib','lib64']:(runtime/name).mkdir(parents=True,exist_ok=True);(runtime/name).chmod(0o755)
base=Path('/lib/x86_64-linux-gnu') if arch=='x86_64' else Path('/usr/aarch64-linux-gnu/lib')
loader='/lib64/ld-linux-x86-64.so.2' if arch=='x86_64' else '/lib/ld-linux-aarch64.so.1'
inputs=[(base/'libc.so.6','lib/libc.so.6'),(base/'libm.so.6','lib/libm.so.6'),(Path('/lib64/ld-linux-x86-64.so.2') if arch=='x86_64' else base/'ld-linux-aarch64.so.1',loader.lstrip('/'))]
records=[]
for source,name in inputs:
 source=source.resolve(strict=True);st=source.stat();assert st.st_uid==0 and not st.st_mode&0o022
 data=source.read_bytes();assert data[:4]==b'\x7fELF' and struct.unpack_from('<H',data,18)[0]==machine
 dest=runtime/name;shutil.copyfile(source,dest);dest.chmod(0o555 if 'ld-linux' in name else 0o444)
 records.append({'path':'/'+name,'source':str(source),'sha256':hashlib.sha256(data).hexdigest()})
archive=r/'third_party/lua/lua-5.4.9.tar.gz';assert hashlib.sha256(archive.read_bytes()).hexdigest()=='2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6'
src=r/f'build/{arch}/r33-lua/lua-5.4.9/src';payload=bundle/'etc/nova-packages/lua-dynamic'
cmd=[cc,'-O2','-DLUA_USE_POSIX','-Wl,-rpath,/lib','-Wl,--disable-new-dtags',*[p.name for p in sorted(src.glob('*.c')) if p.name!='luac.c'],'-lm','-o',str(payload)]
subprocess.run(cmd,cwd=src,check=True);payload.chmod(0o555)
headers=subprocess.check_output(['readelf','-l',str(payload)],text=True);assert loader in headers
needed=subprocess.check_output(['readelf','-d',str(payload)],text=True)
import re
# AArch64 glibc also records the already-shipped interpreter as DT_NEEDED.
expected={'libm.so.6','libc.so.6'} | ({Path(loader).name} if arch=='arm64' else set())
assert set(re.findall(r'Shared library: \[(.*?)\]',needed))==expected
notice=bundle/'Source/glibc-runtime';notice.mkdir(parents=True,exist_ok=True)
shutil.copyfile('/usr/share/doc/libc6/copyright',notice/'copyright')
(notice/'README.txt').write_text('Local development runtime from installed Ubuntu architecture toolchain. LGPL obligations, matching source and relink materials must be reviewed before external distribution. This is not a self-contained SDK or general Linux compatibility guarantee.\n')
out=r/f'build/{arch}/r36-runtime';out.mkdir(parents=True,exist_ok=True)
result={'status':'BUILD_PASS','arch':arch,'interpreter':loader,'runtime_files':records,'dynamic_lua_sha256':hashlib.sha256(payload.read_bytes()).hexdigest(),'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],'scope':'local toolchain runtime; no network; same terminal UID/policy; guest pending'}
(out/'result.json').write_text(json.dumps(result,indent=2));print('DYNAMIC_RUNTIME_BUILD_PASS',arch)

# Fixed alias preserves package verification and quoted user arguments.
alias=bundle/"bin"/'lua-dynamic';content='#!/bin/sh\nexec /bin/nova-pkg run lua-dynamic "$@"\n'
if alias.exists() or alias.is_symlink():
 assert not alias.is_symlink() and alias.read_text()==content
else:
 with alias.open("x") as f:f.write(content)
alias.chmod(0o555)
