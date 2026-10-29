#!/usr/bin/env python3
"""Stage a separate browser runtime; never alter an installed system or guest disk.
ELF resolution uses readelf, never executes an input object. Host-derived files
are hash inventoried but are NOT represented as a signed/released guest package.
"""
import hashlib,json,re,shutil,subprocess,time
from pathlib import Path
P=Path(__file__).resolve().parents[1]
B=P/'build/browser-host'; S=B/'sysroot'
private=json.loads((P/'evidence/r47/private-xcb-build.json').read_text())
assert hashlib.sha256(Path(private['library']).read_bytes()).hexdigest()==private['sha256'], 'private X11 library missing or modified'
out=B/('runtime-'+str(time.time_ns())); out.mkdir(mode=0o755)
manifest=[]; queue=[]; seen=set(); missing=[]
cache={}
for line in subprocess.check_output(['/sbin/ldconfig','-p'],text=True).splitlines():
 m=re.match(r'\s*(\S+) .*=> (\S+)',line)
 if m and 'x86-64' in line:cache.setdefault(m[1],Path(m[2]))
def copy(src,dest):
 src=Path(src); dest=out/str(dest).lstrip('/')
 if dest.exists():return
 dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src.resolve(),dest)
 dest.chmod(0o555 if src.stat().st_mode&0o111 else 0o444)
 manifest.append({'path':'/'+str(dest.relative_to(out)),'source':str(src),'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
 with dest.open('rb') as f:elf=f.read(4)==b'\x7fELF'
 if elf:queue.append(src)
for f in (B/'firefox').rglob('*'):
 if f.is_file():copy(f,Path('/opt/firefox')/f.relative_to(B/'firefox'))
copy('/bin/dash','/bin/sh')
copy(B/'nova-browser-bridge','/usr/bin/nova-browser-bridge')
for name in ['Xvfb','xkbcomp']:copy(S/'usr/bin'/name,'/usr/bin/'+name)
# Runtime dlopen modules do not appear in DT_NEEDED. Explicitly include the
# supported GTK3/software rendering path; discovery completeness remains tested.
for name in ['libgtk-3.so.0','libgdk-3.so.0','libdbus-glib-1.so.2','libnss3.so','libnspr4.so','libasound.so.2','libEGL.so.1','libGL.so.1']:
 local=S/'usr/lib/x86_64-linux-gnu'/name
 if local.exists():copy(local,Path('/usr/lib/x86_64-linux-gnu')/name)
 elif name in cache:copy(cache[name],cache[name])
 else:missing.append(name)
while queue:
 f=queue.pop()
 if str(f) in seen:continue
 seen.add(str(f)); text=subprocess.check_output(['readelf','-d',str(f)],text=True,stderr=subprocess.DEVNULL)
 for name in re.findall(r'\(NEEDED\).*?\[(.*?)\]',text):
  own=B/'firefox'/name
  if own.exists():continue
  candidates=[S/'usr/lib/x86_64-linux-gnu'/name,S/'lib/x86_64-linux-gnu'/name]
  lib=next((v for v in candidates if v.exists()),cache.get(name))
  if lib is None:missing.append(name);continue
  # sysroot paths must be mapped back to their runtime location.
  dest=Path('/')/lib.relative_to(S) if lib.is_relative_to(S) else lib
  copy(lib,dest)
copy('/lib64/ld-linux-x86-64.so.2','/lib64/ld-linux-x86-64.so.2')
for tree in ['/usr/share/X11/xkb','/usr/share/fonts/truetype/dejavu','/etc/fonts','/usr/share/glib-2.0/schemas']:
 for f in Path(tree).rglob('*'):
  if f.is_file():copy(f,f)
copy('/etc/ssl/certs/ca-certificates.crt','/etc/ssl/certs/ca-certificates.crt')
copy(B/'apt-source/libxcb-source/COPYING','/usr/share/licenses/libxcb/COPYING')
copy(P/'evidence/r47/private-xcb.patch','/usr/share/licenses/libxcb/nova-private-x11.patch')
for license in (S/'usr/share/doc').glob('*/copyright'):
 copy(license,Path('/usr/share/doc')/license.relative_to(S/'usr/share/doc'))

for d in ['tmp','proc','dev','home/nova','run']: (out/d).mkdir(parents=True,exist_ok=True)
(out/'etc/ld.so.conf').write_text('/lib/x86_64-linux-gnu\n/usr/lib/x86_64-linux-gnu\n')
# The cache is generated only inside this unmounted, new staging directory.
subprocess.run(['/sbin/ldconfig','-r',str(out)],check=True)
report={'runtime':str(out),'files':len(manifest),'bytes':sum(f.stat().st_size for f in out.rglob('*') if f.is_file()),'missing':sorted(set(missing)),'manifest':manifest,'scope':'host-derived staging; signed provenance and guest acceptance pending'}
e=P/'evidence/r47';(e/'runtime-stage.json').write_text(json.dumps(report,indent=2))
print(json.dumps({k:v for k,v in report.items() if k!='manifest'},indent=2))
raise SystemExit(bool(missing))
