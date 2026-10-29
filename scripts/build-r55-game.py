from pathlib import Path
import sys,json,hashlib,subprocess,shutil
r=Path(__file__).resolve().parents[1];arch=sys.argv[1] if len(sys.argv)>1 else 'x86_64'
if arch not in ('x86_64','arm64'):raise SystemExit('Unsupported architecture')
src=r/'third_party/2048';manifest=json.loads((src/'UPSTREAM.json').read_text())
for n,v in manifest['files'].items():
 if hashlib.sha256((src/n).read_bytes()).hexdigest()!=v['sha256']:raise SystemExit('Upstream digest mismatch: '+n)
b=r/f'build/{arch}/r25-terminal/System/Terminal';payload=b/'etc/nova-packages/2048';payload.parent.mkdir(parents=True,exist_ok=True)
cc='gcc' if arch=='x86_64' else 'aarch64-linux-gnu-gcc'
work=r/f'build/{arch}/r55-game';work.mkdir(parents=True,exist_ok=True)
adapted=work/'2048-nova.c';shutil.copyfile(src/'2048.c',adapted)
subprocess.run(['patch','--batch','--forward',str(adapted),str(src/'nova-terminal.patch')],check=True,stdout=subprocess.DEVNULL)
obj=work/'2048.o';common=['-std=c99','-O2','-Wall','-Wextra','-Werror','-fstack-protector-strong']
subprocess.run([cc,*common,'-Dmain=nova_upstream_game_main','-c',str(adapted),'-o',str(obj)],check=True)
subprocess.run([cc,*common,'-static',str(r/'src/terminal/nova_game_entry.c'),str(obj),'-o',str(payload)],check=True)
license=b/'Source/2048';license.mkdir(parents=True,exist_ok=True)
for n in ['2048.c','LICENSE','UPSTREAM.json','nova-terminal.patch']:shutil.copyfile(src/n,license/n)
shutil.copyfile(adapted,license/'2048-nova.c');shutil.copyfile(r/'src/terminal/nova_game_entry.c',license/'nova_game_entry.c')
alias=b/'bin/2048';alias.parent.mkdir(parents=True,exist_ok=True);content='#!/bin/sh\nexec /bin/nova-pkg run 2048 "$@"\n'
if alias.exists() or alias.is_symlink():
 if alias.is_symlink() or alias.read_text()!=content:raise SystemExit('Existing 2048 command preserved; review required')
else:alias.write_text(content);alias.chmod(0o555)
out=r/f'build/{arch}/r55-game';out.mkdir(parents=True,exist_ok=True)
(out/'result.json').write_text(json.dumps({'status':'BUILD_PASS','arch':arch,'upstream_commit':manifest['commit'],'binary_sha256':hashlib.sha256(payload.read_bytes()).hexdigest(),'license':'MIT','network':'NOT GRANTED','guest':'PENDING'},indent=2))
print('2048_BUILD_PASS',arch)
