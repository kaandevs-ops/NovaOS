from pathlib import Path
import socket,struct,subprocess,os,tempfile,json,array
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='network-auth-',dir=r/'evidence/r37'));root=out/'Network';(root/'etc').mkdir(parents=True);root.chmod(0o755)
for name,text in [('hosts','127.0.0.1 blocked.example\n'),('nsswitch.conf','hosts: files\n'),('http-enabled','http-public-read-v1\n')]:p=root/'etc'/name;p.write_text(text);p.chmod(0o444)
worker=out/'broker';subprocess.run(['gcc','-O2','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_NET_ROOT="{root}"',str(r/'src/network/nova_net_broker.c'),'-o',str(worker)],check=True)
results={};baseline=len(os.listdir('/proc/self/fd'))
def case(name,uid=12004,version=1,length_delta=0,pass_fd=False,grant=True,want_reply=False):
 a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(5)
 actions=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
 if b.fileno()!=3:actions.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
 pid=os.posix_spawn(str(worker),[str(worker)],{},file_actions=actions);b.close()
 ready=a.recv(4096)
 if not grant:assert not ready,name
 else:
  assert len(ready)==20 and struct.unpack('<4Ii',ready)==(0x4e485450,1,0,0,0),name
  child=os.fork()
  if not child:
   try:
    os.setgroups([]);os.setgid(uid);os.setuid(uid)
    url=b'http://blocked.example/';packet=struct.pack('<4Ii',0x4e485450,version,123,len(url)+length_delta,0)+url
    if pass_fd:
     fd=os.open('/dev/null',os.O_RDONLY);a.sendmsg([packet],[(socket.SOL_SOCKET,socket.SCM_RIGHTS,array.array('i',[fd]))]);os.close(fd)
    else:a.send(packet)
    reply=a.recv(4096)
    if want_reply:
     assert len(reply)==20 and struct.unpack('<4Ii',reply)[4]==-13
    else:assert not reply
    os._exit(0)
   except BaseException:os._exit(1)
  assert os.waitpid(child,0)[1]==0,name
 a.close();os.waitpid(pid,0);results[name]='PASS'
case('root-message-rejected',uid=0)
case('wrong-uid-rejected',uid=12006)
case('bad-version-rejected',version=99)
case('length-mismatch-rejected',length_delta=1)
case('unexpected-fd-rejected',pass_fd=True)
case('authorized-client-private-ip-denied',want_reply=True)
(root/'etc/http-enabled').chmod(0o666);case('writable-grant-rejected',grant=False);(root/'etc/http-enabled').chmod(0o444)
(root/'etc/http-enabled').rename(root/'etc/grant-saved');case('missing-grant-rejected',grant=False);(root/'etc/grant-saved').rename(root/'etc/http-enabled')
root.chmod(0o777);case('writable-root-rejected',grant=False);root.chmod(0o755)
assert len(os.listdir('/proc/self/fd'))==baseline;results['parent-fd-stability']='PASS'
record={'status':'PASS','checks':results,'path':str(out),'scope':'real private broker startup, kernel SCM_CREDENTIALS UID checks, ancillary FD rejection, signed-grant/root validation; no external Internet'}
(r/'evidence/r37/network-auth-result.json').write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2))
