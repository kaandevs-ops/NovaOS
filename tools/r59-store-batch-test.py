from pathlib import Path
import os,socket,struct,time,json,subprocess,shutil,tempfile,ssl,threading,http.server,hashlib
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding,PublicFormat
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='store-',dir=r/'evidence/r59'));runtime=out/'runtime';shutil.copytree(r/'build/r59-store-runtime',runtime,symlinks=True)
# copytree intentionally resets test-only ownership; restore virtual home identity.
os.chown(runtime/'home/nova',12004,12004);os.chown(runtime/'home/nova/.nova-apps',12004,12004)
home=out/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
import re
rows=re.findall(r'\{("[^{}]+)\}',(r/'build/x86_64/r33-catalog/catalog.h').read_text());packages={};lines=[]
for row in rows:
 v=re.findall(r'"([^"\n]*)"',row);data=(r/'build/x86_64/r25-terminal/System/Terminal'/v[3].lstrip('/')).read_bytes();packages['/'+v[0]]=data;lines.append(f'{v[0]}\t{v[1]}\tx86_64\t{len(data)}\t{v[4]}\thttps://localhost/{v[0]}\n')
key=Ed25519PrivateKey.generate();body=(f'NOVA-REPO-1\t1\t{int(time.time())+3600}\n'+''.join(lines)).encode();(runtime/'etc/repo.pub').write_bytes(key.public_key().public_bytes(Encoding.Raw,PublicFormat.Raw));(runtime/'etc/repository.nri').write_bytes(key.sign(body)+body)

for f in [runtime/'etc/repo.pub',runtime/'etc/repository.nri']:f.chmod(0o444)
cert=out/'test.pem';priv=out/'test.key';subprocess.run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-keyout',str(priv),'-out',str(cert),'-days','1','-subj','/CN=localhost','-addext','subjectAltName=DNS:localhost'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);priv.chmod(0o600);shutil.copyfile(cert,runtime/'etc/ssl/certs/ca-certificates.crt');(runtime/'etc/ssl/certs/ca-certificates.crt').chmod(0o444)
mode={'slow':False,'bad':False}
class Handler(http.server.BaseHTTPRequestHandler):
 def do_GET(self):
  payload=packages[self.path]
  self.send_response(200);self.send_header('Content-Length',str(len(payload)));self.end_headers()
  try:
   for i in range(0,len(payload),1024):self.wfile.write((b'x'*len(payload) if mode['bad'] else payload)[i:i+1024]);self.wfile.flush();time.sleep(.0001)
  except (BrokenPipeError,ConnectionResetError,ssl.SSLError):pass
 def log_message(self,*args):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',443),Handler);ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(cert,priv);server.socket=ctx.wrap_socket(server.socket,server_side=True);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
helper=out/'helper';cmd=['gcc','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-static','-DNR_TEST_LOOPBACK=1',f'-DNOVA_STORE_ROOT="{runtime}"',f'-DNOVA_STORE_HOME="{home}"',f'-DNOVA_PACKAGE_CATALOG="{r}/build/x86_64/r33-catalog/catalog.h"','src/software/nova_store_session.c','src/software/nova_repository.c','-lcrypto','-ldl','-pthread','-o',str(helper)];subprocess.run(cmd,cwd=r,check=True,stdout=subprocess.DEVNULL,stderr=(out/'build.log').open('w'))

software=out/'software';bundle=r/'build/x86_64/r25-terminal/System/Terminal';subprocess.run(['gcc','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{out}"',f'-DNOVA_TERM_ROOT="{out}/software-root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_software_session.c'),'-o',str(software)],check=True)
exe=out/'ui';subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections','-Isrc/runtime',f'-DNOVA_STORE_HELPER="{helper}"',f'-DNOVA_SOFTWARE_HELPER="{software}"','tests/r59-store-batch.c','src/runtime/nova_runtime.c','-Wl,--gc-sections','-o',str(exe)],cwd=r,check=True)
try:
 x=subprocess.run([str(exe)],cwd=r,capture_output=True,text=True,timeout=120);(out/'ui.log').write_text(x.stdout+x.stderr);print(out);print(x.stdout+x.stderr);assert x.returncode==0,x.returncode
finally:server.shutdown();server.server_close();thread.join()
