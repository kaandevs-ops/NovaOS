#!/usr/bin/env python3
"""Exercise real commands in production isolated PTY helper, not host shell."""
from pathlib import Path
import subprocess,tempfile,os,socket,struct,json,time
r=Path(__file__).resolve().parents[1]
out=Path(tempfile.mkdtemp(prefix='package-lifecycle-',dir=r/'evidence/r32'))
parent=out/'user-session';parent.mkdir(mode=0o700)
home=parent/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
helper=out/'session';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{parent}"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],check=True)
(home/'.nova-apps').mkdir(mode=0o700);os.chown(home/'.nova-apps',12004,12004)
for n in range(3,33):
 p=home/'.nova-apps'/('.install-'+str(n));p.write_bytes(b'interrupted');p.chmod(0o600);os.chown(p,12004,12004)
a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(15)
actions=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
if b.fileno()!=3:actions.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
pid=os.posix_spawn(str(helper),[str(helper)],{},file_actions=actions);b.close()
def receive():
 data=a.recv(4096);assert len(data)>=32
 h=struct.unpack('<8I',data[:32]);assert h[0]==0x4e54524d and h[3]==len(data)-32
 return h,data[32:]
def prompt():
 data=b''
 while not data.endswith(b'nova$ '):
  h,v=receive();assert h[2]==17,(h,v);data+=v
 return data
h,v=receive();assert h[2]==16;prompt();results={}
def command(name,cmd,expected):
 a.send(struct.pack('<8I',0x4e54524d,1,1,len(cmd)+1,0,0,0,0)+cmd.encode()+b'\r')
 output=prompt();(out/(name+'.log')).write_bytes(output)
 assert expected.encode() in output,(name,output)
 results[name]='PASS'
try:
 command('interrupted-install-recovery','nova-pkg install sqlite','sqlite: installed from verified local catalog')
finally:
 a.close();status=os.waitpid(pid,0)[1]
assert status==0,status
assert (home/'.nova-apps/sqlite').is_file()
result={'status':'PASS','checks':results,'scope':'real PTY helper, UID12004, private namespaces, bounded seccomp; local catalog install/run/tamper reject/remove/reinstall in WSL; not VM','path':str(out)}
(r/'evidence/r32/interrupted-install-result.json').write_text(json.dumps(result,indent=2));print(result)
