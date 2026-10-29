from pathlib import Path
import subprocess,sys,json,hashlib
r=Path(__file__).resolve().parents[1];arch=sys.argv[1];assert arch in ('x86_64','arm64')
cc='gcc' if arch=='x86_64' else 'aarch64-linux-gnu-gcc'
dest=r/f'build/{arch}/r25-terminal/System/Terminal/bin/nova-edit'
subprocess.run([cc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fstack-protector-strong',str(r/'src/terminal/nova_edit.c'),'-o',str(dest)],check=True)
out=r/f'build/{arch}/r42-editor';out.mkdir(parents=True,exist_ok=True)
(out/'result.json').write_text(json.dumps({'arch':arch,'status':'BUILD_PASS','sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'guest':'PENDING'},indent=2))
print('R42_EDITOR_BUILD_PASS',arch)
