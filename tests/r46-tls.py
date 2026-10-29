from pathlib import Path
import subprocess,ssl,socket,threading,time,json,datetime,tempfile,os
from cryptography import x509
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='tls-',dir=r/'evidence/r46'));out.chmod(0o700)
exe=out/'driver';cmd=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=address,undefined','-Isrc/network','tests/r46-tls-driver.c','src/network/nova_tls_transport.c','-lssl','-lcrypto','-o',str(exe)]
subprocess.run(cmd,cwd=r,check=True)
now=datetime.datetime.now(datetime.timezone.utc)
def key():return rsa.generate_private_key(public_exponent=65537,key_size=2048)
ca_key=key();name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'NovaOS disposable TLS TEST CA')]);ca=x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(ca_key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(now-datetime.timedelta(days=1)).not_valid_after(now+datetime.timedelta(days=2)).add_extension(x509.BasicConstraints(ca=True,path_length=0),True).sign(ca_key,hashes.SHA256());(out/'ca.pem').write_bytes(ca.public_bytes(serialization.Encoding.PEM));(out/'invalid.pem').write_text('not a CA\n')
def leaf(expired=False):
 k=key();cert=x509.CertificateBuilder().subject_name(x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'test.nova.invalid')])).issuer_name(name).public_key(k.public_key()).serial_number(x509.random_serial_number()).not_valid_before(now-datetime.timedelta(days=3)).not_valid_after(now-datetime.timedelta(days=1) if expired else now+datetime.timedelta(days=1)).add_extension(x509.SubjectAlternativeName([x509.DNSName('test.nova.invalid')]),False).add_extension(x509.BasicConstraints(ca=False,path_length=None),True).sign(ca_key,hashes.SHA256());prefix='expired' if expired else 'valid';(out/(prefix+'.pem')).write_bytes(cert.public_bytes(serialization.Encoding.PEM));f=out/(prefix+'.key');f.write_bytes(k.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()));f.chmod(0o600);return prefix
leaf();leaf(True)
rogue_key=key();rogue_name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'Untrusted disposable CA')]);rogue=x509.CertificateBuilder().subject_name(rogue_name).issuer_name(rogue_name).public_key(rogue_key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(now-datetime.timedelta(days=1)).not_valid_after(now+datetime.timedelta(days=1)).add_extension(x509.BasicConstraints(ca=True,path_length=0),True).sign(rogue_key,hashes.SHA256());(out/'untrusted.pem').write_bytes(rogue.public_bytes(serialization.Encoding.PEM));results={}
def run(label,hostname='test.nova.invalid',prefix='valid',trust='ca.pem',stall=False,truncated=False):
 a,b=socket.socketpair();errors=[]
 def server():
  try:
   if stall:time.sleep(.6);return
   if truncated:b.sendall(b'garbage');return
   ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(out/(prefix+'.pem'),out/(prefix+'.key'))
   with ctx.wrap_socket(b,server_side=True) as tls:
    tls.settimeout(2);request=tls.recv(4096);assert request.startswith(b'GET /robots.txt HTTP/1.1\r\n');tls.sendall(b'HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nNOVA');
    try:tls.unwrap()
    except (ssl.SSLError,OSError):pass
  except (ssl.SSLError,OSError):pass
  except Exception as e:errors.append(str(e))
  finally:b.close()
 th=threading.Thread(target=server);th.start();start=time.monotonic();q=subprocess.run([str(exe),str(a.fileno()),hostname,str(out/trust),'200' if stall else '2000'],pass_fds=(a.fileno(),),capture_output=True,text=True,timeout=5);a.close();th.join();assert q.returncode==0,(label,q.stderr);value=json.loads(q.stdout);assert value['fd_preserved'],value;assert not errors,errors
 if label=='trusted_valid':assert value['rc']==0 and value['http_ok'],value
 else:assert value['rc']<0,value
 if stall:assert value['rc']==-110 and time.monotonic()-start<2,value
 results[label]=value
run('trusted_valid');run('wrong_hostname',hostname='wrong.nova.invalid');run('expired',prefix='expired');run('invalid_ca',trust='invalid.pem');run('untrusted_ca',trust='untrusted.pem');run('stalled_timeout',stall=True);run('malformed_tls',truncated=True);run('hostname_injection',hostname='test.nova.invalid\r\nInjected: x')
results['scope']='Real TLS sockets in WSL + ASAN/UBSAN; not NovaOS broker/guest/browser; disposable TLS test keys stay mode600 in private evidence directory.'
(out/'results.json').write_text(json.dumps(results,indent=2));(r/'evidence/r46/tls-result.json').write_text(json.dumps({'status':'PASS','cases':8,'path':str(out),'guest':'PENDING','browser':'PENDING'},indent=2));print(json.dumps(results,indent=2))
