from pathlib import Path
import os,socket,struct,time,json,subprocess,shutil,tempfile,ssl,threading,http.server,hashlib
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding,PublicFormat
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='store-',dir=r/'evidence/r59'));runtime=out/'runtime';shutil.copytree(r/'build/r59-store-runtime',runtime,symlinks=True)
# copytree intentionally resets test-only ownership; restore virtual home identity.
os.chown(runtime/'home/nova',12004,12004);os.chown(runtime/'home/nova/.nova-apps',12004,12004)
home=out/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
(home/'private-note.txt').write_text('private user document; must not be exposed to downloader');os.chown(home/'private-note.txt',12004,12004)
payload=(r/'build/x86_64/r25-terminal/System/Terminal/etc/nova-packages/2048').read_bytes();key=Ed25519PrivateKey.generate();body=f'NOVA-REPO-1\t1\t{int(time.time())+3600}\n2048\t1.0.3\tx86_64\t{len(payload)}\t{hashlib.sha256(payload).hexdigest()}\thttps://localhost/payload\n'.encode();(runtime/'etc/repo.pub').write_bytes(key.public_key().public_bytes(Encoding.Raw,PublicFormat.Raw));(runtime/'etc/repository.nri').write_bytes(key.sign(body)+body)
for f in [runtime/'etc/repo.pub',runtime/'etc/repository.nri']:f.chmod(0o444)
cert=out/'test.pem';priv=out/'test.key';subprocess.run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-keyout',str(priv),'-out',str(cert),'-days','1','-subj','/CN=localhost','-addext','subjectAltName=DNS:localhost'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);priv.chmod(0o600);shutil.copyfile(cert,runtime/'etc/ssl/certs/ca-certificates.crt');(runtime/'etc/ssl/certs/ca-certificates.crt').chmod(0o444)
mode={'slow':False,'bad':False}
class Handler(http.server.BaseHTTPRequestHandler):
 def do_GET(self):
  self.send_response(200);self.send_header('Content-Length',str(len(payload)));self.end_headers()
  try:
   for i in range(0,len(payload),1024):self.wfile.write((b'x'*len(payload) if mode['bad'] else payload)[i:i+1024]);self.wfile.flush();time.sleep(.04 if mode['slow'] else 0)
  except (BrokenPipeError,ConnectionResetError,ssl.SSLError):pass
 def log_message(self,*args):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',443),Handler);ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(cert,priv);server.socket=ctx.wrap_socket(server.socket,server_side=True);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
helper=out/'helper';cmd=['gcc','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-static','-DNR_TEST_LOOPBACK=1',f'-DNOVA_STORE_ROOT="{runtime}"',f'-DNOVA_STORE_HOME="{home}"',f'-DNOVA_PACKAGE_CATALOG="{r}/build/x86_64/r33-catalog/catalog.h"','src/software/nova_store_session.c','src/software/nova_repository.c','-lcrypto','-ldl','-pthread','-o',str(helper)];subprocess.run(cmd,cwd=r,check=True,stdout=subprocess.DEVNULL,stderr=(out/'build.log').open('w'))
a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(20);ops=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)];
if b.fileno()!=3:ops.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
pid=os.posix_spawn(str(helper),[str(helper)],{},file_actions=ops);b.close();tag=0;results={}
def recv():
 d=a.recv(128);assert len(d)==32,('early close',d);p=struct.unpack('<8I',d);assert p[0]==0x4e535452 and p[1]==1;return p
def send(op,index=3):
 global tag
 tag+=1;a.send(struct.pack('<8I',0x4e535452,1,op,tag,index,0,0,0));return tag
def done(t):
 progress=[]
 while True:
  p=recv();assert p[3]==t,p
  if p[2]==18:return p,progress
  assert p[2]==17 and p[5]<=p[6];progress.append(p[5])
try:
 p=recv();assert p[2]==16 and p[7]==8,p;results['signed-catalog-ready']='PASS'
 status=Path(f'/proc/{pid}/status').read_text();assert 'Uid:\t12004\t12004' in status and 'NoNewPrivs:\t1' in status and 'Seccomp:\t2' in status;results['real-uid-nnp-seccomp']='PASS'
 assert not Path(f'/proc/{pid}/root/home/nova/private-note.txt').exists();assert (home/'private-note.txt').is_file();results['no-general-user-home-grant']='PASS'
 mounts=Path(f'/proc/{pid}/mountinfo').read_text();assert any('/home/nova/.nova-apps ' in l and 'noexec' in l for l in mounts.splitlines());results['package-store-noexec']='PASS'
 trace=subprocess.Popen(['strace','-ff','-p',str(pid),'-o',str(out/'trace')],stdout=subprocess.DEVNULL,stderr=(out/'trace.stderr').open('w'));time.sleep(.1)
 mode['bad']=True;t=send(1);p,progress=done(t);assert p[7]==74,p;assert not (home/'.nova-apps/2048').exists();results['hash-failure-no-install']='PASS'
 mode['bad']=False;mode['slow']=True;t=send(1);p=recv();assert p[2]==17
 while not p[5]:p=recv();assert p[2]==17,p
 send(2);p,progress=done(t);assert p[7]==125,p;assert not (home/'.nova-apps/2048').exists();results['real-download-cancel']='PASS'
 mode['slow']=False;t=send(1);p,progress=done(t);assert p[7]==0,(p,progress);assert (home/'.nova-apps/2048').read_bytes()==payload;assert progress;results['https-verified-atomic-install']='PASS'
 send(2);results['late-cancel-after-commit-tolerated']='PASS'
 t=send(1,0);p,progress=done(t);assert p[7]==95;results['uncatalogued-id-denied']='PASS'
finally:
 a.close();_,st=os.waitpid(pid,0);results['session-exit']=str(os.waitstatus_to_exitcode(st));server.shutdown();server.server_close();thread.join();(out/'results.json').write_text(json.dumps(results,indent=2));print(out);print(json.dumps(results,indent=2))
