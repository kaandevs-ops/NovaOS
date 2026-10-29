#!/usr/bin/env python3
from pathlib import Path
import subprocess,tempfile,os,socket,struct,json,time
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='software-',dir=r/'evidence/r58'))
parent=out/'user-session';parent.mkdir(mode=0o700);home=parent/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
helper=out/'helper';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
subprocess.run(['gcc','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{parent}"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_software_session.c'),'-o',str(helper)],check=True)
a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(20);actions=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
if b.fileno()!=3:actions.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
pid=os.posix_spawn(str(helper),[str(helper)],{},file_actions=actions);b.close();tag=0;results={};output={i:b'' for i in range(4)}
def recv():
 d=a.recv(2048);assert len(d)>=32,('early disconnect',d);h=struct.unpack('<8I',d[:32]);assert h[0]==0x4e535746 and len(d)==32+h[5],h
 if h[2]==18:output[h[4]]+=d[32:]
 return h,d[32:]
def send(op,i=0,data=b''):
 global tag
 tag+=1;a.send(struct.pack('<8I',0x4e535746,1,op,tag,i,len(data),0,0)+data);return tag
def action(name,op,i=0):
 t=send(op,i)
 while True:
  h,d=recv()
  if h[2]==17 and h[3]==t:
   state=struct.unpack('<8I',d);assert state[5]==0,(name,state);results[name]='PASS';return state
try:
 h,d=recv();assert h[2]==16 and h[6]==12004;results['ready-user']='PASS'
 s=action('list-absent',1);assert s[:4]==(3,3,3,3),s
 action('install-sqlite',2);s=action('verify-sqlite',1);assert s[0]==0
 action('run-sqlite',4);time.sleep(.3);send(5,0,b'select 6*7;\r')
 while b'42' not in output[0]:recv()
 results['real-sqlite-query']='PASS'
 action('install-lua-concurrent',2,1);action('run-lua-concurrent',4,1);time.sleep(.3);send(5,1,b'print(12345)\r')
 while b'12345' not in output[1]:recv()
 results['independent-pty']='PASS'
 s=action('two-running',1);assert s[4]&3==3,s
 t=send(3,0)
 while True:
  h,d=recv()
  if h[2]==17 and h[3]==t:assert struct.unpack('<8I',d)[5]==16;break
 results['reject-remove-running']='PASS'
 for i in [0,1]:
  action('close-'+str(i),7,i)
  while True:
   h,d=recv()
   if h[2]==19 and h[4]==i:break
 action('install-game',2,3);action('run-game',4,3)
 while b'2048.c' not in output[3]:recv()
 send(5,3,b'\x1b[C');time.sleep(.1);send(5,3,b'q')
 while b'QUIT?' not in output[3]:recv()
 send(5,3,b'y')
 while True:
  h,d=recv()
  if h[2]==19 and h[4]==3:break
 results['real-game-arrows-quit']='PASS';action('remove-game',3,3)
 action('remove-sqlite',3);action('remove-lua',3,1);s=action('final-absent',1);assert s[:4]==(3,3,3,3),s
finally:
 print({i:v.decode(errors='replace') for i,v in output.items()},flush=True);a.close();_,status=os.waitpid(pid,0);results['helper-exit']=str(os.waitstatus_to_exitcode(status));(out/'results.json').write_text(json.dumps(results,indent=2));print(out);print(json.dumps(results,indent=2))
