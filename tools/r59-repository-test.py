from pathlib import Path
import ctypes as C,subprocess,hashlib,json,time,os,ssl,threading,http.server,tempfile,errno
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding,PublicFormat
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='repo-',dir=r/'evidence/r59'))
for name,extra in [('repository',[]),('repository-test',['-DNR_TEST_LOOPBACK=1'])]:
 subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-shared','-fPIC','-fsanitize=undefined','-fno-sanitize-recover=all',*extra,'src/software/nova_repository.c','-lcrypto','-o',str(out/(name+'.so'))],cwd=r,check=True)
class Entry(C.Structure):_fields_=[('id',C.c_char*49),('version',C.c_char*33),('arch',C.c_char*8),('sha256',C.c_char*65),('url',C.c_char*513),('bytes',C.c_uint64)]
class Index(C.Structure):_fields_=[('serial',C.c_uint64),('expires',C.c_uint64),('count',C.c_uint),('entries',Entry*64)]
lib=C.CDLL(str(out/'repository.so'));test=C.CDLL(str(out/'repository-test.so'))
for l in [lib,test]:
 l.nr_publish_index.argtypes=[C.c_int,C.c_void_p,C.c_size_t,C.c_void_p,C.c_char_p,C.c_uint64,C.c_uint64]
 l.nr_verify.argtypes=[C.c_void_p,C.c_size_t,C.c_void_p,C.c_char_p,C.c_uint64,C.c_uint64,C.POINTER(Index)];l.nr_https_fetch.argtypes=[C.POINTER(Entry),C.c_char_p,C.c_int]
key=Ed25519PrivateKey.generate();pub=key.public_key().public_bytes(Encoding.Raw,PublicFormat.Raw);now=int(time.time());results={}
line='game-test\t1.0\tx86_64\t4\t'+'0'*64+'\thttps://example.org/game\n'
def verify(name,body,expected=0,mutate=False,floor=1,arch=b'x86_64'):
 wire=key.sign(body)+body
 if mutate:wire=wire[:-1]+b'x'
 index=Index();rc=lib.nr_verify(wire,len(wire),pub,arch,floor,now,C.byref(index));assert rc==expected,(name,rc,expected);assert index.count==(1 if not expected else 0);results[name]='PASS'
def body(rows=line,serial=2,expires=now+3600):return f'NOVA-REPO-1\t{serial}\t{expires}\n'.encode()+rows.encode()
verify('signature-valid',body());verify('tampered-signature',body(),errno.EBADMSG,True);verify('expired',body(expires=now),errno.ETIME);verify('rollback',body(serial=1),errno.ESTALE,floor=2)
verify('wrong-arch',body(),errno.EPROTO,arch=b'arm64');verify('duplicate-id',body(line+line),errno.EPROTO);verify('path-id',body(line.replace('game-test','../game')),errno.EPROTO);verify('plaintext-url',body(line.replace('https:','http:')),errno.EPROTO);verify('userinfo-url',body(line.replace('example.org','user@example.org')),errno.EPROTO);verify('bad-size',body(line.replace('\t4\t','\t999999999\t')),errno.EPROTO);verify('nul-body',body()+b'\x00',errno.EPROTO)
cache=out/'cache';cache.mkdir(mode=0o700);df=os.open(cache,os.O_RDONLY|os.O_DIRECTORY)
def publish(name,serial,expected=0,expires=now+3600,rows=line):
 b=body(rows,serial,expires);w=key.sign(b)+b;old=(cache/'repository.nri').read_bytes() if (cache/'repository.nri').exists() else None;rc=lib.nr_publish_index(df,w,len(w),pub,b'x86_64',1,now);assert rc==expected,(name,rc,expected)
 if rc:assert (cache/'repository.nri').read_bytes()==old
 else:assert (cache/'repository.nri').read_bytes()==w
 results[name]='PASS'
publish('atomic-first-index',2);publish('atomic-next-index',3);publish('persisted-rollback-rejected',2,errno.ESTALE);publish('expired-update-preserves-old',4,errno.ETIME,expires=now);publish('same-serial-changed-rejected',3,errno.ESTALE,expires=now+3601);publish('same-index-idempotent',3);os.close(df)
# Ephemeral test TLS certificate, never guest/UEFI signing material.
cert=out/'localhost.pem';tlskey=out/'localhost.key';subprocess.run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-keyout',str(tlskey),'-out',str(cert),'-days','1','-subj','/CN=localhost','-addext','subjectAltName=DNS:localhost'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL);tlskey.chmod(0o600)
payload=b'NovaOS real TLS payload\n'
class Handler(http.server.BaseHTTPRequestHandler):
 def do_GET(self):
  if self.path=='/redirect':self.send_response(302);self.send_header('Location','https://localhost/payload');self.end_headers();return
  self.send_response(200);self.send_header('Content-Length',str(len(payload)));self.end_headers();self.wfile.write(payload)
 def log_message(self,*args):pass
server=http.server.HTTPServer(('127.0.0.1',443),Handler);context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.load_cert_chain(cert,tlskey);server.socket=context.wrap_socket(server.socket,server_side=True);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
def fetch(name,url='https://localhost/payload',expected=0,digest=None,size=None,ca=cert,production=False):
 e=Entry(id=b'game-test',version=b'1.0',arch=b'x86_64',sha256=(digest or hashlib.sha256(payload).hexdigest()).encode(),url=url.encode(),bytes=len(payload) if size is None else size)
 fd=os.open(out/(name+'.stage'),os.O_CREAT|os.O_EXCL|os.O_RDWR,0o600)
 try:
  rc=(lib if production else test).nr_https_fetch(C.byref(e),str(ca).encode(),fd);assert rc==expected,(name,rc,expected)
  if not rc:assert os.read(fd,65536)==payload
  else:assert os.fstat(fd).st_size==0
 finally:os.close(fd)
 results[name]='PASS'
try:
 fetch('real-tls-and-sha');fetch('wrong-digest',expected=errno.EBADMSG,digest='0'*64);fetch('oversized-payload',expected=errno.EIO,size=4);fetch('redirect-not-followed',url='https://localhost/redirect',expected=errno.EBADMSG);fetch('untrusted-certificate',ca=Path('/etc/ssl/certs/ca-certificates.crt'),expected=errno.EIO);fetch('private-ip-production-rejected',expected=errno.EACCES,production=True)
 payload=(r/'build/x86_64/r25-terminal/System/Terminal/etc/nova-packages/2048').read_bytes()
 gamehash=hashlib.sha256(payload).hexdigest();gamebody=body(f'2048\t1.0.3\tx86_64\t{len(payload)}\t{gamehash}\thttps://localhost/payload\n');wire=key.sign(gamebody)+gamebody;verified=Index();assert lib.nr_verify(wire,len(wire),pub,b'x86_64',1,now,C.byref(verified))==0
 fd=os.open(out/'verified-game.stage',os.O_CREAT|os.O_EXCL|os.O_RDWR,0o600)
 try:assert test.nr_https_fetch(C.byref(verified.entries[0]),str(cert).encode(),fd)==0;assert os.read(fd,len(payload))==payload
 finally:os.close(fd)
 results['signed-index-to-real-game-TLS']='PASS';(r/'evidence/r59/downloaded-game.json').write_text(json.dumps({'path':str(out/'verified-game.stage'),'sha256':gamehash,'origin':'real local HTTPS test server with ephemeral trusted test CA; not public production repository'},indent=2))

finally:server.shutdown();server.server_close();thread.join()
# Real public HTTPS request: licensed upstream game source, not an installed executable.
url='https://raw.githubusercontent.com/gabrielecirulli/2048/master/LICENSE.txt';reference=subprocess.check_output(['/usr/bin/curl','--disable','--proto','=https','--fail','--silent','--max-time','30',url]);payload=reference
fetch('public-upstream-game-license',url=url,production=True,ca=Path('/etc/ssl/certs/ca-certificates.crt'))
(out/'results.json').write_text(json.dumps(results,indent=2));print(out);print(json.dumps(results,indent=2))
