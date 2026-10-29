from pathlib import Path
import os,socket,struct,subprocess,tempfile,json,array
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='negative-',dir=r/'evidence/r58'));p=out/'user-session';p.mkdir(mode=0o700);h=p/'home';h.mkdir(mode=0o700);os.chown(h,12004,12004);helper=out/'helper';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
subprocess.run(['gcc','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{p}"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_software_session.c'),'-o',str(helper)],check=True)
def start():
 a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(10);ops=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
 if b.fileno()!=3:ops.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
 pid=os.posix_spawn(str(helper),[str(helper)],{},file_actions=ops);b.close();d=a.recv(2048);assert struct.unpack('<8I',d)[2]==16;return a,pid
results={};base=len(os.listdir('/proc/self/fd'))
def packet(op=1,tag=1,index=0,length=0,reserved=0):return struct.pack('<8I',0x4e535746,1,op,tag,index,length,0,reserved)
for name,data in [('unknown-op',packet(99)),('unknown-app',packet(index=4)),('zero-tag',packet(tag=0)),('reserved',packet(reserved=1)),('truncated',packet(length=8)),('oversized',packet(length=2048)+b'x'*2048),('bad-resize',packet(op=6,length=8)+struct.pack('<2I',999,999))]:
 a,pid=start();a.send(data);assert a.recv(2048)==b'';a.close();_,s=os.waitpid(pid,0);assert os.waitstatus_to_exitcode(s)!=0;results[name]='PASS'
a,pid=start();a.send(packet());reply=a.recv(2048);assert struct.unpack('<8I',reply[:32])[2]==17;a.send(packet());assert a.recv(2048)==b'';a.close();os.waitpid(pid,0);results['replayed-tag']='PASS'
a,pid=start();fd=os.open('/dev/null',os.O_RDONLY);a.sendmsg([packet()],[(socket.SOL_SOCKET,socket.SCM_RIGHTS,array.array('i',[fd]))]);os.close(fd);assert a.recv(2048)==b'';a.close();os.waitpid(pid,0);results['unsolicited-fd']='PASS'
assert len(os.listdir('/proc/self/fd'))==base;results['parent-fd-cleanup']='PASS';(out/'results.json').write_text(json.dumps(results,indent=2));print(out);print(json.dumps(results,indent=2))
