from pathlib import Path
import hashlib,json,re
r=Path(__file__).resolve().parents[1];old=r.parent/'phase19-desktop-r23-virtualbox-test-20261019'
paths=['src/runtime/nova_surface_protocol.h','src/runtime/nova_app_catalog_protocol.h','src/runtime/nova_app_metadata_protocol.h','src/runtime/nova_app_protocol.h','src/runtime/nova_app_sandbox.h','src/runtime/nova_app_security_protocol.h','files/src/nova_files_core.c','files/src/nova_files_guest_metadata.h']
checks={}
for name in paths:
 a=(old/name).read_bytes();b=(r/name).read_bytes();assert a==b;checks[name]=hashlib.sha256(b).hexdigest()
a=(old/'src/init/main.c').read_text();b=(r/'src/init/main.c').read_text()
def function(s,name):
 start=re.search(r'static[^\n]*\b'+name+r'\(',s).start();opening=s.index('{',start);depth=1;i=opening+1
 while depth:
  if s[i]=='{':depth+=1
  elif s[i]=='}':depth-=1
  i+=1
 return s[start:i]
for name in ['sandbox_install_desktop_seccomp','sandbox_install_files_seccomp','sandbox_delegate_files_directory','apply_application_sandbox','sandbox_build_private_root','sandbox_drop_application_privileges']:
 x=function(a,name);y=function(b,name);assert x==y;checks['init-function/'+name]=hashlib.sha256(y.encode()).hexdigest()
out={'status':'PASS','scope':'selected unchanged protocol/core/grant/seccomp/namespace authorities, not whole init byte identity','sha256':checks};(r/'evidence/r24/security-source-identity.json').write_text(json.dumps(out,indent=2));print('security authorities unchanged',len(checks))
