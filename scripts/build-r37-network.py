from pathlib import Path
import subprocess,sys,json,hashlib,os
r=Path(__file__).resolve().parents[1];arch=sys.argv[1];assert arch in ('x86_64','arm64');cc='gcc' if arch=='x86_64' else 'aarch64-linux-gnu-gcc'
b=r/f'build/{arch}/r25-terminal/System';(b/'Core').mkdir(parents=True,exist_ok=True);(b/'Terminal/bin').mkdir(parents=True,exist_ok=True)
root=b/'Network';(root/'etc').mkdir(parents=True,exist_ok=True);root.chmod(0o755)
for name,text in [('resolv.conf','nameserver 10.0.2.3\noptions timeout:1 attempts:1\n'),('nsswitch.conf','hosts: files dns\n'),('hosts','127.0.0.1 localhost\n')]:
 p=root/'etc'/name;p.write_text(text);p.chmod(0o444)
results={}
for src,dest in [('nova_http.c',b/'Terminal/bin/nova-http'),('nova_net_broker.c',b/'Core/nova-net-broker'),('nova_net_setup.c',b/'Core/nova-net-setup')]:
 subprocess.run([cc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fstack-protector-strong',str(r/'src/network'/src),'-o',str(dest)],check=True)
 results[src]=hashlib.sha256(dest.read_bytes()).hexdigest()
out=r/f'build/{arch}/r37-network';out.mkdir(parents=True,exist_ok=True);(out/'result.json').write_text(json.dumps({'status':'BUILD_PASS','arch':arch,'sha256':results,'scope':'bounded plain HTTP; no TLS; guest pending'},indent=2));print('NETWORK_BUILD_PASS',arch)

if os.environ.get('NOVA_R37_TEST_NETWORK')=='1':
 p=root/'etc/test-nat.enabled';p.write_text('virtualbox-nat-test-v1\n');p.chmod(0o444)
 p=root/'etc/http-enabled';p.write_text('http-public-read-v1\n');p.chmod(0o444)
elif (root/'etc/test-nat.enabled').exists() or (root/'etc/http-enabled').exists():
 raise SystemExit('Refusing stale test-network marker in non-test build')
