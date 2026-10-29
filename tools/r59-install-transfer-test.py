#!/usr/bin/env python3
"""Exercise real commands in production isolated PTY helper, not host shell."""
from pathlib import Path
import subprocess,tempfile,os,socket,struct,json,time
r=Path(__file__).resolve().parents[1]
out=Path(tempfile.mkdtemp(prefix='package-lifecycle-',dir=r/'evidence/r59'))
parent=out/'user-session';parent.mkdir(mode=0o700)
home=parent/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
helper=out/'session';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{parent}"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],check=True)
header=r/'build/x86_64/r33-catalog/catalog.h'
subprocess.run(['gcc','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',f'-DNOVA_PACKAGE_CATALOG="{header}"',str(r/'src/terminal/nova_pkg.c'),str(r/'src/runtime/nova_sha256.c'),'-o',str(bundle/'bin/nova-pkg')],check=True)
baseline_fds=len(os.listdir('/proc/self/fd'))
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
download=json.loads((r/'evidence/r59/downloaded-game.json').read_text());shutil=__import__('shutil');shutil.copyfile(download['path'],home/'downloaded.game');os.chown(home/'downloaded.game',12004,12004)
try:
 command('prepare-transfer','cat downloaded.game > transfer.game','nova$ ')
 command('install-from-fd','nova-pkg install-fd 2048 3 3<transfer.game','2048: installed')
 command('verified-run','nova-pkg run 2048 test','All 13 tests executed successfully')
 command('reject-duplicate','nova-pkg install-fd 2048 3 3<transfer.game; echo RESULT_$?','RESULT_1')
 command('existing-app-preserved','nova-pkg run 2048 test','All 13 tests executed successfully')
 command('remove','nova-pkg remove 2048','2048: removed')
 command('tampered-transfer',"printf CORRUPT > bad.game; nova-pkg install-fd 2048 3 3<bad.game; echo RESULT_$?",'RESULT_1')
 command('atomic-failure','nova-pkg status 2048 --machine; echo RESULT_$?','RESULT_3')
 command('invalid-fd','nova-pkg install-fd 2048 0; echo RESULT_$?','RESULT_2')
 command('unknown-id','nova-pkg install-fd ../../etc 3 3<transfer.game; echo RESULT_$?','RESULT_2')
 command('local-regression','nova-pkg install sqlite; nova-pkg run sqlite -list :memory: "select 42;"','42')
finally:
 a.close();os.waitpid(pid,0)
assert len(os.listdir('/proc/self/fd'))==baseline_fds
(out/'results.json').write_text(json.dumps(results,indent=2));print(out);print(json.dumps(results,indent=2))
