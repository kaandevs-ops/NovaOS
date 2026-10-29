from pathlib import Path
import socket,ipaddress,subprocess,json,time,ssl
r=Path(__file__).resolve().parents[1];out=r/'evidence/r46';exe=r/'build/r46/tls-public-probe';exe.parent.mkdir(exist_ok=True)
subprocess.run(['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Isrc/network','tests/r46-tls-driver.c','src/network/nova_tls_transport.c','-lssl','-lcrypto','-o',str(exe)],cwd=r,check=True)
host='www.google.com';result={'scope':'WSL public TLS transport probe, NOT guest/GUI browser','host':host,'path':'/robots.txt','status':'PENDING'}
try:
 ca=ssl.get_default_verify_paths().cafile;assert ca and Path(ca).is_file()
 addresses=socket.getaddrinfo(host,443,socket.AF_INET,socket.SOCK_STREAM);conn=None
 for row in addresses:
  address=row[4];assert ipaddress.ip_address(address[0]).is_global
  try:conn=socket.create_connection(address,timeout=5);break
  except OSError:continue
 if conn is None:raise OSError('No reachable public IPv4 address')
 with conn:
  start=time.monotonic();q=subprocess.run([str(exe),str(conn.fileno()),host,ca,'10000'],pass_fds=(conn.fileno(),),capture_output=True,text=True,timeout=15);assert q.returncode==0,q.stderr;value=json.loads(q.stdout);result.update(value);result['elapsed_ms']=round((time.monotonic()-start)*1000,2);result['status']='PASS' if value['rc']==0 and value['http_ok'] else 'FAIL'
except Exception as e:result['environment_error']=str(e)
(out/'public-tls-probe.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
