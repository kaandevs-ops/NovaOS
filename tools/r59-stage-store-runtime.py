from pathlib import Path
import sys,shutil,subprocess,re,json,os
r=Path(__file__).resolve().parents[1];dest=Path(sys.argv[1]).resolve();assert dest.is_relative_to(r/'build') or dest.is_relative_to(r/'evidence/r59');assert not dest.exists();dest.mkdir(mode=0o755,parents=True)
for p in ['bin','usr/bin','etc/ssl/certs','dev','tmp','home/nova/.nova-apps']:(dest/p).mkdir(parents=True,exist_ok=True)
os.chown(dest/'home/nova',12004,12004);(dest/'home/nova').chmod(0o700);os.chown(dest/'home/nova/.nova-apps',12004,12004);(dest/'home/nova/.nova-apps').chmod(0o700)
cache={}
for line in subprocess.check_output(['/sbin/ldconfig','-p'],text=True).splitlines():
 m=re.match(r'\s*(\S+) .*=> (\S+)',line)
 if m and 'x86-64' in line:cache.setdefault(m[1],Path(m[2]))
queue=[];copied=set();records=[];origins=[]
def copy(src,path):
 target=dest/path.lstrip('/')
 if str(target) in copied:return
 target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,target);target.chmod(0o555 if os.access(src,os.X_OK) else 0o444);copied.add(str(target));records.append(path);origins.append(Path(src))
 if target.read_bytes()[:4]==b'\x7fELF':queue.append(target)
copy(Path('/usr/bin/curl'),'/usr/bin/curl');copy(r/'build/x86_64/r25-terminal/System/Terminal/bin/nova-pkg','/bin/nova-pkg')
while queue:
 p=queue.pop();text=subprocess.check_output(['readelf','-d',str(p)],text=True,stderr=subprocess.DEVNULL)
 for name in re.findall(r'\(NEEDED\).*?\[(.*?)\]',text):
  assert name in cache,name;copy(cache[name],str(cache[name]))
copy(Path('/lib64/ld-linux-x86-64.so.2'),'/lib64/ld-linux-x86-64.so.2');copy(Path('/etc/ssl/certs/ca-certificates.crt'),'/etc/ssl/certs/ca-certificates.crt')
(dest/'etc/hosts').write_text('127.0.0.1 localhost\n');(dest/'etc/hosts').chmod(0o444)
# Test NAT configuration is explicit; this is not a general DHCP implementation.
(dest/'etc/resolv.conf').write_text('nameserver 10.0.2.3\noptions timeout:2 attempts:2\n');(dest/'etc/resolv.conf').chmod(0o444)
for name in ['nsswitch.conf']:
 if Path('/etc',name).is_file():copy(Path('/etc',name),'/etc/'+name)
# Carry third-party notices with the runtime; no key material is staged.
packages=set()
for src in origins:
 if src.is_relative_to(r):continue
 candidates=[str(src),str(src.resolve())]
 if str(src).startswith('/lib/'):candidates.append('/usr'+str(src))
 for candidate in candidates:
  q=subprocess.run(['dpkg-query','-S',candidate],capture_output=True,text=True)
  if q.returncode==0:
   packages.add(q.stdout.splitlines()[0].split(': ')[0].split(',')[0].split(':')[0]);break
for package in sorted(packages):
 notice=Path('/usr/share/doc')/package/'copyright'
 if notice.is_file():copy(notice,'/usr/share/doc/'+package+'/copyright')
import hashlib
manifest=[{'path':str(p.relative_to(dest)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in dest.rglob('*') if p.is_file()]
(dest/'runtime-provenance.json').write_text(json.dumps({'files':manifest,'license_packages':sorted(packages),'scope':'host-derived unsigned staging; guest acceptance pending'},indent=2))
subprocess.run(['/sbin/ldconfig' ,'-r',str(dest)],check=True)
print(json.dumps({'runtime':str(dest),'files':len(records),'repository_configured':False,'guest':'PENDING'}))
