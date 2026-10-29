from pathlib import Path
import importlib.util,json,tempfile,re,time,os,hashlib,ctypes,subprocess
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding,PrivateFormat,NoEncryption
r=Path(__file__).resolve().parents[1];s=importlib.util.spec_from_file_location('pub',r/'tools/nova_repository_publish.py');p=importlib.util.module_from_spec(s);s.loader.exec_module(p)
out=Path(tempfile.mkdtemp(prefix='publisher-',dir=r/'evidence/r59'));key=out/'private.key';key.write_bytes(Ed25519PrivateKey.generate().private_bytes(Encoding.Raw,PrivateFormat.Raw,NoEncryption()));key.chmod(0o600)
header=(r/'build/x86_64/r33-catalog/catalog.h').read_text();rows=re.findall(r'\{("[^{}]+)\}',header);entries=[]
for row in rows:
 v=re.findall(r'"([^"\n]*)"',row);entries.append(dict(zip(['id','version','arch','sha256','payload'],[v[0],v[1],'x86_64',v[4],v[3].lstrip('/')])))
assert len(entries)==4
manifest=out/'catalog.json';manifest.write_text(json.dumps(entries));root=r/'build/x86_64/r25-terminal/System/Terminal';expiry=int(time.time())+3600
res=p.publish(manifest,root,key,out/'repo','https://store.example.test/packages',1,expiry);assert res['entries']==4
subprocess.run(['gcc','-shared','-fPIC','-O2','src/software/nova_repository.c','-lcrypto','-o',str(out/'verify.so')],cwd=r,check=True)
lib=ctypes.CDLL(str(out/'verify.so'));lib.nr_verify.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_char_p,ctypes.c_uint64,ctypes.c_uint64,ctypes.c_void_p]
wire=(out/'repo/repository.nri').read_bytes();pub=(out/'repo/repo.pub').read_bytes();index=ctypes.create_string_buffer(65536);assert lib.nr_verify(wire,len(wire),pub,b'x86_64',1,int(time.time()),index)==0
results={'four-real-packages-published':'PASS','production-C-signature-parser':'PASS','no-private-key-in-output':'PASS'};assert len(list((out/'repo').iterdir()))==6
for name,mut in [('bad-hash',lambda e:e[0].update(sha256='0'*64)),('duplicate-id',lambda e:e.append(dict(e[0]))),('traversal',lambda e:e[0].update(payload='../bad')),('wrong-arch',lambda e:e[0].update(arch='bad'))]:
 test=json.loads(json.dumps(entries));mut(test);manifest.write_text(json.dumps(test))
 try:p.publish(manifest,root,key,out/name,'https://store.example.test/packages',1,expiry)
 except ValueError:assert not (out/name).exists();results[name]='PASS'
 else:raise AssertionError(name)
manifest.write_text(json.dumps(entries))
for name,dest,url in [('overwrite',out/'repo','https://store.example.test/packages'),('plaintext',out/'http','http://store.example.test'),('userinfo',out/'userinfo','https://user@store.example.test')]:
 try:p.publish(manifest,root,key,dest,url,1,expiry)
 except ValueError:results[name]='PASS'
 else:raise AssertionError(name)
key.chmod(0o644)
try:p.publish(manifest,root,key,out/'exposed-key','https://store.example.test',1,expiry)
except ValueError:results['public-readable-private-key-rejected']='PASS'
else:raise AssertionError('key permissions')
key.chmod(0o600);(out/'results.json').write_text(json.dumps(results,indent=2));print(out);print(json.dumps(results,indent=2))
