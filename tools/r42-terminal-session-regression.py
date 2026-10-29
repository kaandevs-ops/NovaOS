#!/usr/bin/env python3
from pathlib import Path
import socket,os,subprocess,struct,time,json,tempfile,select,signal
r=Path(__file__).resolve().parents[1];e=r/'evidence/r25';e.mkdir(parents=True,exist_ok=True)
run=Path(tempfile.mkdtemp(prefix='session-',dir=e));binary=run/'session'
bundle=r/'build/x86_64/r25-terminal/System/Terminal'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_TERM_ROOT="{run}/root"',f'-DNOVA_SHARED_WORKSPACE_PARENT="{run}/absent"',f'-DNOVA_WORKSPACE_DATA="{run}/absent-data"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(binary)],check=True)
MAGIC=0x4e54524d

def start():
 a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET)
 actions=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
 if b.fileno()!=3:actions.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
 pid=os.posix_spawn(str(binary),[str(binary)],{},file_actions=actions);b.close();a.settimeout(8)
 return a,pid

def packet(op,data=b'',rows=0,cols=0,magic=MAGIC,reserved=0):return struct.pack('<8I',magic,1,op,len(data),rows,cols,0,reserved)+data

def receive(a):
 data=a.recv(4096)
 if not data:return None,b''
 assert len(data)>=32
 h=struct.unpack('<8I',data[:32]);assert h[0]==MAGIC and h[1]==1 and h[3]==len(data)-32
 return h,data[32:]

def prompt(a):
 out=b''
 while not out.endswith(b'nova$ '):
  h,data=receive(a);assert h and h[2]==17;out+=data
 return out

def command(a,cmd,expected):
 a.send(packet(1,cmd.encode()+b'\r'));out=b'';end=time.monotonic()+8
 while time.monotonic()<end:
  h,data=receive(a);assert h and h[2]!=19,(h,out)
  out+=data
  if os.environ.get('NOVA_TERM_VERBOSE'):print('PTY',repr(data),flush=True)
  # avoid treating echoed command text as successful execution
  if expected in out:return out
 raise AssertionError((cmd,out))

results={};fd_before=len(os.listdir('/proc/self/fd'));a,pid=start();out=b''
try:
 h,data=receive(a);assert h and h[2]==16,(h,data);results['namespace-start']='PASS';out+=prompt(a)
 # Last output includes prompt; run fixed real commands, not canned output.
 out+=command(a,'id; pwd; uname -s',b'Linux\r\n')
 assert b'uid=12004' in out and b'/home/nova\r\n' in out
 results['real-shell-identity']='PASS'
 out+=command(a,"printf 'b\\na\\n' | sort; echo done-pipe",b'\r\ndone-pipe\r\n')
 assert b'a\r\nb\r\n' in out;results['pipeline']='PASS'
 out+=command(a,'echo actual-data > note.txt; cat note.txt',b'\r\nactual-data\r\n');results['real-file-write-read']='PASS'
 out+=command(a,'cat /etc/shadow; echo denied-done',b'\r\ndenied-done\r\n')
 assert b'No such file' in out;results['host-filesystem-isolation']='PASS'
 out+=command(a,'echo bad > /etc/blocked; echo readonly-done',b'\r\nreadonly-done\r\n')
 assert b'Read-only file system' in out;results['readonly-system']='PASS'
 a.send(packet(2,rows=31,cols=93));out+=command(a,'stty size',b'31 93\r\n');results['pty-resize']='PASS'
 a.send(packet(1,b'sleep 30\r'));time.sleep(.1);a.send(packet(1,b'\x03'));time.sleep(.1)
 while select.select([a],[],[],.1)[0]:
  h,data=receive(a);out+=data
 out+=command(a,'echo interrupted-done',b'\r\ninterrupted-done\r\n');results['ctrl-c']='PASS'
 a.send(packet(1,b'sleep 60 &\r'));time.sleep(.1)
 a.send(packet(3));a.close();t=time.monotonic();_,st=os.waitpid(pid,0);print('CLOSE STATUS',st,os.waitstatus_to_exitcode(st),flush=True);assert time.monotonic()-t<3 and os.waitstatus_to_exitcode(st)==0
 results['close-kills-namespace']='PASS'
 a,pid=start();h,data=receive(a);assert h and h[2]==16;out+=prompt(a)
 out+=command(a,'test ! -f note.txt && echo clean-session',b'\r\nclean-session\r\n');results['honest-session-only-home']='PASS'
 a.send(packet(1,b'x',reserved=1))
 for _ in range(8):
  h,data=receive(a)
  if h and h[2]==19:break
  assert h and h[2]==17
 else:raise AssertionError('Missing protocol rejection')
 a.close();os.waitpid(pid,0);results['malformed-packet-rejected']='PASS'
 # Bounded output flood: stop consuming it, then close. Private namespace must
 # still terminate promptly and must not retain background descendants.
 a,pid=start();h,data=receive(a);assert h and h[2]==16;out+=prompt(a)
 a.send(packet(1,b'yes flood\r'));time.sleep(.2);a.close();start_close=time.monotonic();_,st=os.waitpid(pid,0);assert time.monotonic()-start_close<3 and os.waitstatus_to_exitcode(st)==0
 results['output-backpressure-close']='PASS'
 elapsed=[]
 for cycle in range(20):
  started=time.monotonic();a,pid=start();h,data=receive(a);assert h and h[2]==16;out+=prompt(a)
  marker=f'cycle-{cycle:02d}'
  out+=command(a,'echo '+marker,b'\r\n'+marker.encode()+b'\r\n')
  a.close();_,st=os.waitpid(pid,0);assert os.waitstatus_to_exitcode(st)==0;elapsed.append((time.monotonic()-started)*1000)
 assert len(os.listdir('/proc/self/fd'))==fd_before
 results['20-production-session-cycles-fd-stable']='PASS'
 (e/'terminal-cycle-timing.json').write_text(json.dumps({'unit':'ms','scope':'WSL session startup/real command/shutdown, not frame time','samples':elapsed},indent=2))

except BaseException:
 try:a.close()
 except:pass
 try:os.waitpid(pid,0)
 except:pass
 raise
finally:
 (run/'transcript.txt').write_bytes(out);(e/'terminal-session-results.json').write_text(json.dumps({'results':results,'run':str(run),'scope':'real WSL namespaces/PTY/shell; not guest boot'},indent=2))
print(json.dumps(results,indent=2))
