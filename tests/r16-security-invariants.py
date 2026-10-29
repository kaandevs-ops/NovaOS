from pathlib import Path
import re
r=Path(__file__).resolve().parents[1];ref=r.parent/'phase19-desktop-lumina-r13-desktopfirst-build.3xxF8tZo'
def body(s,name):
 start=s.index('static int '+name+'(');a=s.index('{',start);i=a+1;depth=1
 while depth:
  if s[i]=='{':depth+=1
  if s[i]=='}':depth-=1
  i+=1
 return s[start:i]
s=(r/'src/init/main.c').read_text();old=(ref/'src/init/main.c').read_text()
assert body(s,'sandbox_install_desktop_seccomp')==body(old,'sandbox_install_desktop_seccomp')
policy=body(s,'sandbox_install_files_seccomp')
for nr in ['openat','mkdirat','unlinkat','renameat','fchmodat','clone','mount','unshare','setuid']:
 assert f'NOVA_SECCOMP_ALLOW_NR(NOVA_NR_{nr})' not in policy
assert 'F_GETFL only' in policy
assert 'files-private-read-v1' in (r/'configs/services/nova-files.service').read_text()
for name in ['nova_surface_protocol.h','nova_resource_broker_protocol.h']:
 assert (r/'src/runtime'/name).read_bytes()==(ref/'src/runtime'/name).read_bytes()
print('R16_DESKTOP_FILTER_AND_EXISTING_IPC_BYTE_IDENTICAL=PASS')
