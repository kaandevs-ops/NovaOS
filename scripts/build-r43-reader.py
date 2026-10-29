from pathlib import Path
import subprocess,sys,json,hashlib
r=Path(__file__).resolve().parents[1];a=sys.argv[1];assert a in ('x86_64','arm64')
cc='gcc' if a=='x86_64' else 'aarch64-linux-gnu-gcc'
b=r/f'build/{a}/r25-terminal/System/Terminal/bin';assert b.is_dir()
dest=b/'nova-read'
subprocess.run([cc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fstack-protector-strong',str(r/'src/terminal/nova_read.c'),'-o',str(dest)],check=True)
e=r/f'build/{a}/r43-reader';e.mkdir(parents=True,exist_ok=True)
(e/'result.json').write_text(json.dumps({'arch':a,'status':'BUILD_PASS','sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'guest':'PENDING'},indent=2))
print('R43_READER_BUILD_PASS',a)
