#!/usr/bin/env python3
"""Exercise real commands in production isolated PTY helper, not host shell."""
from pathlib import Path
import subprocess,tempfile,os,socket,struct,json,time
r=Path(__file__).resolve().parents[1]
out=Path(tempfile.mkdtemp(prefix='external-app-',dir=r/'evidence/r29'))
parent=out/'user-session';parent.mkdir(mode=0o700)
home=parent/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
helper=out/'session';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{parent}"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],check=True)
a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(15)
actions=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
if b.fileno()!=3:actions.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
pid=os.posix_spawn(str(helper),[str(helper)],{},file_actions=actions);b.close()
def receive():
 data=a.recv(4096);assert len(data)>=32
 h=struct.unpack('<8I',data[:32]);assert h[0]==0x4e54524d and h[3]==len(data)-32
 return h,data[32:]
def prompt():
 data=b''
 while not data.endswith(b'nova$ '):
  h,v=receive();assert h[2]==17,(h,v);data+=v
 return data
h,v=receive();assert h[2]==16;prompt();results={}
def command(name,cmd,expected):
 a.send(struct.pack('<8I',0x4e54524d,1,1,len(cmd)+1,0,0,0,0)+cmd.encode()+b'\r')
 output=prompt();(out/(name+'.log')).write_bytes(output)
 assert expected.encode() in output,(name,output)
 results[name]='PASS'
try:
 command('prepare','mkdir -p work; printf "alpha 2\\nbeta 3\\n" > work/data.txt; echo PREPARED','\r\nPREPARED\r\n')
 command('native-info','nova-info','Identity: uid=12004 gid=12004')
 command('sqlite-version',"sqlite3 -batch -noheader -list :memory: 'select sqlite_version();'",'\r\n3.54.1\r\n')
 command('sqlite-write',"sqlite3 -batch -noheader -list notes.db \"create table notes(id integer primary key, body text); insert into notes(body) values('NovaOS real data'); select body from notes;\"",'\r\nNovaOS real data\r\n')
 command('sqlite-integrity',"sqlite3 -batch -noheader -list notes.db 'pragma integrity_check;'",'\r\nok\r\n')
 command('sqlite-readonly-system',"sqlite3 -batch -noheader -list /bin/forbidden.db 'create table x(a);'; test ! -e /bin/forbidden.db && echo DB_ISOLATION_OK",'\r\nDB_ISOLATION_OK\r\n')
 command('find' ,'find work -name data.txt','\r\nwork/data.txt\r\n')
 command('awk',"awk '{s += $2} END {print s}' work/data.txt",'\r\n5\r\n')
 command('sha256','sha256sum work/data.txt > check.txt; sha256sum -c check.txt','work/data.txt: OK')
 command('archive','tar -czf data.tar.gz work/data.txt && mkdir unpack && tar -xzf data.tar.gz -C unpack && cmp work/data.txt unpack/work/data.txt && echo ARCHIVE_OK','\r\nARCHIVE_OK\r\n')
 command('base64','base64 work/data.txt | base64 -d > decoded; cmp work/data.txt decoded && echo BASE64_OK','\r\nBASE64_OK\r\n')
 command('diff','diff work/data.txt decoded && echo DIFF_OK','\r\nDIFF_OK\r\n')
 command('xargs',"printf 'one\\ntwo\\n' | xargs echo",'\r\none two\r\n')
 command('readonly-system','echo forbidden > /bin/forbidden; test ! -e /bin/forbidden && echo READONLY_OK','\r\nREADONLY_OK\r\n')
finally:
 a.close();status=os.waitpid(pid,0)[1]
assert status==0,status
assert (home/'unpack/work/data.txt').read_bytes()==b'alpha 2\nbeta 3\n'
assert subprocess.check_output([str(bundle/'bin/sqlite3'),'-batch','-noheader','-list',str(home/'notes.db'),'select body from notes;'],text=True).strip()=='NovaOS real data'
results['database-survives-session-close']='PASS'
result={'status':'PASS','checks':results,'scope':'actual production PTY helper, mount/PID/net namespaces, UID12004 under current R32 terminal policy (positional I/O plus restricted executable memfd); WSL, not VM','path':str(out)}
(r/'evidence/r29/external-app-result.json').write_text(json.dumps(result,indent=2));print(result)
