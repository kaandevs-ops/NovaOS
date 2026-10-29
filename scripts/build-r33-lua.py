from pathlib import Path
import hashlib,tarfile,subprocess,sys,shutil,json
r=Path(__file__).resolve().parents[1];arch=sys.argv[1];assert arch in ('x86_64','arm64')
source=r/'third_party/lua/lua-5.4.9.tar.gz';assert hashlib.sha256(source.read_bytes()).hexdigest()=='2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6'
out=r/f'build/{arch}/r33-lua';out.mkdir(parents=True,exist_ok=True)
if not (out/'lua-5.4.9').exists():
 with tarfile.open(source) as t:
  for m in t:
   assert m.name=='lua-5.4.9' or m.name.startswith('lua-5.4.9/')
   assert '..' not in Path(m.name).parts and not m.issym() and not m.islnk()
  t.extractall(out,filter='data')
src=out/'lua-5.4.9/src';cc='gcc' if arch=='x86_64' else 'aarch64-linux-gnu-gcc'
cmd=[cc,'-O2','-static','-DLUA_USE_POSIX',*[p.name for p in sorted(src.glob('*.c')) if p.name!='luac.c'],'-lm','-o',str(out/'lua')]
with (out/'compile.log').open('w') as f:subprocess.run(cmd,cwd=src,check=True,stdout=f,stderr=subprocess.STDOUT)
bundle=r/f'build/{arch}/r25-terminal/System/Terminal';payload=bundle/'etc/nova-packages/lua';payload.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(out/'lua',payload);payload.chmod(0o555)
notice=bundle/'Source/lua';notice.mkdir(parents=True,exist_ok=True);shutil.copy2(source,notice/source.name);shutil.copy2(out/'lua-5.4.9/doc/readme.html',notice/'readme-license.html')
(out/'result.json').write_text(json.dumps({'status':'BUILD_PASS','arch':arch,'version':'5.4.9','sha256':hashlib.sha256(payload.read_bytes()).hexdigest(),'native_dynamic_modules':'NOT_ENABLED','guest':'PENDING'},indent=2));print('LUA_BUILD_PASS',arch)

# Fixed alias preserves package verification and quoted user arguments.
alias=bundle/"bin"/'lua';content='#!/bin/sh\nexec /bin/nova-pkg run lua "$@"\n'
if alias.exists() or alias.is_symlink():
 assert not alias.is_symlink() and alias.read_text()==content
else:
 with alias.open("x") as f:f.write(content)
alias.chmod(0o555)
