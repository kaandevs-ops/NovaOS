"""Pinned offline dependency build. No signing, guest, mount or key access."""
from pathlib import Path
import hashlib,json,sys,tarfile,subprocess
r=Path(__file__).resolve().parents[1];a=sys.argv[1];assert a in ('x86_64','arm64')
archive=r/'third_party/mupdf/mupdf-1.28.5-source.tar.gz'
assert hashlib.sha256(archive.read_bytes()).hexdigest()=='98a5c10cda20c3992cdf76ff6b2a1149c32bd79cc796d3f703230b1185b7e934'
base=r/'build/deps';base.mkdir(parents=True,exist_ok=True);src=base/'mupdf-1.28.5-source'
if not src.exists():
 with tarfile.open(archive) as t:
  for m in t.getmembers():
   assert not m.name.startswith('/') and '..' not in Path(m.name).parts and Path(m.name).parts[0]=='mupdf-1.28.5-source'
  t.extractall(base,filter='data')
output=r/f'build/{"x86_64" if a=="x86_64" else "aarch64"}/mupdf'
cmd=['make','-j4','libs','build=small',f'OUT={output}','html=no','mujs=no','xps=no','svg=no','extract=no','brotli=no','tofu_cjk=yes','HAVE_X11=no','HAVE_GLUT=no','HAVE_CURL=no','USE_SYSTEM_LIBS=no','threading=no']
if a=='arm64':cmd+=['CC=aarch64-linux-gnu-gcc','CXX=clang++ --target=aarch64-linux-gnu -nostdinc++','AR=aarch64-linux-gnu-ar','LD=aarch64-linux-gnu-ld']
subprocess.run(cmd,cwd=src,check=True)
