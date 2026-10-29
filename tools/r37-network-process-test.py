from pathlib import Path
import os,subprocess,tempfile,json,sys,socket,struct,threading,http.server,time
r=Path(__file__).resolve().parents[1]
if os.environ.get('NOVA_NETWORK_TEST_NAMESPACE')!='1':
 cp=subprocess.run(['unshare','--net','--',sys.executable,str(Path(__file__).resolve())],env={**os.environ,'NOVA_NETWORK_TEST_NAMESPACE':'1'});raise SystemExit(cp.returncode)
subprocess.run(['ip','link','set','lo','up'],check=True)
subprocess.run(['ip','addr','add','93.184.216.34/32','dev','lo'],check=True)
subprocess.run(['ip','addr','add','93.184.216.35/32','dev','lo'],check=True)
out=Path(tempfile.mkdtemp(prefix='http-process-',dir=r/'evidence/r37'));bundle=out/'bundle'
import shutil
shutil.copytree(r/'build/x86_64/r25-terminal/System/Terminal',bundle,symlinks=True)
root=out/'Network';(root/'etc').mkdir(parents=True);root.chmod(0o755)
(root/'etc/hosts').write_text('93.184.216.34 fixture.example\n127.0.0.1 blocked.example\n93.184.216.35 refused.example\n')
(root/'etc/nsswitch.conf').write_text('hosts: files\n');(root/'etc/resolv.conf').write_text('nameserver 127.0.0.1\noptions timeout:1 attempts:1\n')
(root/'etc/http-enabled').write_text('http-public-read-v1\n')
for p in (root/'etc').iterdir():p.chmod(0o444)
broker=out/'broker';helper=out/'helper'
common=['gcc','-std=c11','-O2','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation']
subprocess.run(common+[f'-DNOVA_NET_ROOT="{root}"',str(r/'src/network/nova_net_broker.c'),'-o',str(broker)],check=True)
subprocess.run(common+[f'-DNOVA_NET_BROKER="{broker}"',f'-DNOVA_SHARED_WORKSPACE_PARENT="{out}/absent"',f'-DNOVA_WORKSPACE_DATA="{out}/absent-data"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],check=True)
class Handler(http.server.BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_GET(self):
  if self.path=='/redirect':self.send_response(302);self.send_header('Location','http://blocked.example/');self.end_headers();return
  if self.path=='/slow':time.sleep(4)
  if self.path=='/large':body=b'Z'*70000
  else:body=b'NOVA_REAL_HTTP_BODY\n'
  self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers()
  try:self.wfile.write(body)
  except BrokenPipeError:pass
server=http.server.ThreadingHTTPServer(('93.184.216.34',80),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(20)
actions=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
if b.fileno()!=3:actions.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
pid=os.posix_spawn(str(helper),[str(helper)],{},file_actions=actions);b.close()
def receive():
 data=a.recv(4096);h=struct.unpack('<8I',data[:32]);assert h[0]==0x4e54524d;return h,data[32:]
def prompt():
 text=b''
 while not text.endswith(b'nova$ '):
  h,d=receive();assert h[2]==17,(h,d);text+=d
 return text
h,d=receive();assert h[2]==16,(h,d);prompt();results={}
def command(name,cmd,expected):
 a.send(struct.pack('<8I',0x4e54524d,1,1,len(cmd)+1,0,0,0,0)+cmd.encode()+b'\r');data=prompt();(out/(name+'.log')).write_bytes(data);assert expected.encode() in data,(name,data);results[name]='PASS'
try:
 command('connection-error-preserved','nova-http http://refused.example/','Connection refused (-111)')
 command('real-http-body','nova-http http://fixture.example/ok','\r\nNOVA_REAL_HTTP_BODY\r\n')
 command('redirect-rejected','if nova-http http://fixture.example/redirect; then echo BAD; else echo REDIRECT_REJECTED; fi','\r\nREDIRECT_REJECTED\r\n')
 command('large-body-rejected','if nova-http http://fixture.example/large; then echo BAD; else echo LARGE_REJECTED; fi','\r\nLARGE_REJECTED\r\n')
 command('private-dns-rejected','if nova-http http://blocked.example/; then echo BAD; else echo PRIVATE_REJECTED; fi','\r\nPRIVATE_REJECTED\r\n')
 command('https-not-faked','if nova-http https://fixture.example/; then echo BAD; else echo TLS_UNSUPPORTED; fi','\r\nTLS_UNSUPPORTED\r\n')
 command('timeout-rejected','if nova-http http://fixture.example/slow; then echo BAD; else echo TIMEOUT_REJECTED; fi','\r\nTIMEOUT_REJECTED\r\n')
 command('works-after-timeout','nova-http http://fixture.example/ok','\r\nNOVA_REAL_HTTP_BODY\r\n')
 command('download-file','nova-http http://fixture.example/ok > fetched.txt; cat fetched.txt','\r\nNOVA_REAL_HTTP_BODY\r\n')
 command('20-fetch-cycles','ok=1; for i in $(seq 1 20); do nova-http http://fixture.example/ok >/dev/null || ok=0; done; echo FETCH_$ok','\r\nFETCH_1\r\n')
finally:a.close();status=os.waitpid(pid,0)[1];server.shutdown()
assert status==0,status
import time
for attempt in range(20):
 alive=[]
 for proc in Path('/proc').glob('[0-9]*/cmdline'):
  try:
   if str(broker).encode() in proc.read_bytes():alive.append(str(proc))
  except (FileNotFoundError,PermissionError):pass
 if not alive:break
 time.sleep(0.05)
assert not alive,alive
results['broker-exits-with-terminal']='PASS'
record={'status':'PASS','checks':results,'path':str(out),'scope':'real production terminal UID12004 private net namespace + separate UID12005 HTTP broker; real TCP server on public-format IP in isolated test-only namespace; no host network changes or external Internet'}
(r/'evidence/r37/network-process-result.json').write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2))
