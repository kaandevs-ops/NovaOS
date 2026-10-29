from pathlib import Path
import subprocess,tempfile,os,socket,struct,time,json
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='sqlite-reopen-',dir=r/'evidence/r29'));parent=out/'user-session';parent.mkdir(mode=0o700);home=parent/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
helper=out/'session';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{parent}"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],check=True)
def packet(op,data=b''):return struct.pack('<8I',0x4e54524d,1,op,len(data),0,0,0,0)+data
def read(a):
 b=a.recv(4096);assert len(b)>=32;h=struct.unpack('<8I',b[:32]);assert h[0]==0x4e54524d and h[3]==len(b)-32;return h,b[32:]
def prompt(a):
 data=b''
 while not data.endswith(b'nova$ '):
  h,b=read(a);assert h[2]==17,(h,b);data+=b
 return data
for cycle in range(2):
 a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(8)
 actions=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
 if b.fileno()!=3:actions.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
 pid=os.posix_spawn(str(helper),[str(helper)],{},file_actions=actions);b.close();h,data=read(a);assert h[2]==16 and h[6]==(12004|0x10000),h;prompt(a)
 if cycle==0:a.send(packet(1,b'sqlite3 -batch -noheader -list work.db \"create table data(v); insert into data values(42);\"\r'));prompt(a)
 a.send(packet(1,b"sqlite3 -batch -noheader -list work.db 'select v from data;'\r"));assert b'\r\n42\r\n' in prompt(a)
 a.close();assert os.waitpid(pid,0)[1]==0
assert (home/'work.db').is_file()
result={'status':'PASS','scope':'real private PTY helper with preexisting shared ext4 directory and mount/PID namespaces in WSL; host persistence is not VM reboot evidence','cycles':2,'sqlite_close_reopen':'PASS','path':str(out)};(r/'evidence/r29/sqlite-reopen-result.json').write_text(json.dumps(result,indent=2));print(result)
