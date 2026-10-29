#!/usr/bin/env python3
"""Exercise real commands in production isolated PTY helper, not host shell."""
from pathlib import Path
import subprocess,tempfile,os,socket,struct,json,time,shutil,hashlib
r=Path(__file__).resolve().parents[1]
out=Path(tempfile.mkdtemp(prefix='package-lifecycle-',dir=r/'evidence/r34'))
parent=out/'user-session';parent.mkdir(mode=0o700)
home=parent/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
helper=out/'session';bundle=out/'bundle'
shutil.copytree(r/'build/x86_64/r25-terminal/System/Terminal',bundle)
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{parent}"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],check=True)
baseline_fds=len(os.listdir('/proc/self/fd'))
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
 command('unknown-operation','if nova-pkg nonsense sqlite; then echo UNEXPECTED; else echo BAD_OP_REJECTED; fi','\r\nBAD_OP_REJECTED\r\n')
 command('filesystem-capacity-command','df -k /home/nova > /tmp/capacity && echo FILESYSTEM_STATS_OK','\r\nFILESYSTEM_STATS_OK\r\n')
 command('catalog','nova-pkg list','sqlite 3.54.1')
 command('not-installed','nova-pkg status sqlite','sqlite: not installed')
 command('run-before-install',"if nova-pkg run sqlite :memory: 'select 1;'; then echo UNEXPECTED; else echo NO_INSTALL_REJECTED; fi",'\r\nNO_INSTALL_REJECTED\r\n')
 command('install','nova-pkg install sqlite','sqlite: installed from verified local catalog')
 command('verified-status','nova-pkg status sqlite','sqlite: installed, verified')
 command('run',"nova-pkg run sqlite -batch -noheader -list :memory: 'select sqlite_version();'",'\r\n3.54.1\r\n')
 command('database',"nova-pkg run sqlite -list managed.db \"create table notes(v); insert into notes values(123); select v from notes;\"",'\r\n123\r\n')
 payload=bundle/'etc/nova-packages/sqlite3'
 trusted=payload.read_bytes();before=hashlib.sha256((home/'.nova-apps/sqlite').read_bytes()).hexdigest()
 try:
  payload.write_bytes(b'broken-catalog-payload')
  command('upgrade-bad-source','if nova-pkg upgrade sqlite; then echo UNEXPECTED; else echo BAD_SOURCE_REJECTED; fi','\r\nBAD_SOURCE_REJECTED\r\n')
  assert hashlib.sha256((home/'.nova-apps/sqlite').read_bytes()).hexdigest()==before
  assert not list((home/'.nova-apps').glob('.install-*'))
  results['failed-upgrade-preserves-old-image']='PASS'
 finally:payload.write_bytes(trusted)
 command('upgrade-current','nova-pkg upgrade sqlite','sqlite: upgraded atomically')
 command('upgrade-preserves-database',"nova-pkg run sqlite -list managed.db 'select v from notes;'",'\r\n123\r\n')
 command('upgrade-stale-image','chmod u+w .nova-apps/sqlite; echo previous-build > .nova-apps/sqlite; nova-pkg upgrade sqlite','sqlite: upgraded atomically')
 command('upgrade-stale-exec',"nova-pkg run sqlite -list managed.db 'select v from notes;'",'\r\n123\r\n')
 command('upgrade-symlink','nova-pkg remove sqlite; echo KEEP > sentinel; ln -s ../sentinel .nova-apps/sqlite; if nova-pkg upgrade sqlite; then echo UNEXPECTED; else echo SYMLINK_REJECTED; fi','\r\nSYMLINK_REJECTED\r\n')
 command('upgrade-symlink-target','cat sentinel; rm .nova-apps/sqlite; ln sentinel .nova-apps/sqlite; if nova-pkg upgrade sqlite; then echo UNEXPECTED; else echo HARDLINK_REJECTED; fi','\r\nHARDLINK_REJECTED\r\n')
 command('upgrade-hardlink-target','cat sentinel; rm .nova-apps/sqlite; if nova-pkg upgrade sqlite; then echo UNEXPECTED; else echo MISSING_REJECTED; fi','\r\nMISSING_REJECTED\r\n')
 command('upgrade-restore','nova-pkg install sqlite','sqlite: installed from verified local catalog')
 command('upgrade-cycle',"ok=1; for i in $(seq 1 20); do nova-pkg upgrade sqlite >/dev/null && nova-pkg run sqlite -list managed.db 'select v from notes;' >/dev/null || ok=0; done; echo UPGRADE_CYCLES_$ok",'\r\nUPGRADE_CYCLES_1\r\n')
 command('lua-catalog','nova-pkg list','lua 5.4.9')
 command('lua-install','nova-pkg install lua','lua: installed from verified local catalog')
 command('lua-version','nova-pkg run lua -v','Lua 5.4.9')
 command('lua-math',"nova-pkg run lua -e 'local s=0; for i=1,100 do s=s+i end; print(s)'",'\r\n5050\r\n')
 command('user-script','printf \'%s\\n\' \'local f=assert(io.open("script-output.txt","w")); for i=1,5 do f:write(i*i,"\\n") end; f:close(); print("SCRIPT_OK")\' > demo.lua; nova-pkg run lua demo.lua','\r\nSCRIPT_OK\r\n')
 command('lua-pipe','nova-pkg run lua -e \'local p=assert(io.popen("echo LUA_PIPE_OK")); print(p:read("*l")); assert(p:close())\'','\r\nLUA_PIPE_OK\r\n')
 command('lua-readonly','nova-pkg run lua -e \'assert(io.open("/bin/forbidden","w")==nil); print("LUA_READONLY_OK")\'','\r\nLUA_READONLY_OK\r\n')
 command('lua-script-error','nova-pkg run lua -e \'error("EXPECTED_LUA_FAILURE")\'; echo LUA_SESSION_ALIVE','\r\nLUA_SESSION_ALIVE\r\n')
 command('lua-no-dynamic-module','nova-pkg run lua -e \'local f,e=package.loadlib("/etc/no-module.so","luaopen_x"); assert(f==nil and e:match("not enabled")); print("NATIVE_MODULES_DISABLED")\'','\r\nNATIVE_MODULES_DISABLED\r\n')
 command('cross-package-swap','chmod u+w .nova-apps/lua; cat .nova-apps/sqlite > .nova-apps/lua; nova-pkg status lua','installed package invalid: Bad message')
 command('lua-reinstall','nova-pkg remove lua; nova-pkg install lua','lua: installed from verified local catalog')
 command('sqlite-independent',"nova-pkg run sqlite -list managed.db 'select v from notes;'",'\r\n123\r\n')
 command('lua-remove','nova-pkg remove lua','lua: removed; user databases preserved')
 command('duplicate-install','if nova-pkg install sqlite; then echo UNEXPECTED; else echo DUPLICATE_REJECTED; fi','\r\nDUPLICATE_REJECTED\r\n')
 command('unknown-id','if nova-pkg install ../../etc; then echo UNEXPECTED; else echo UNKNOWN_REJECTED; fi','\r\nUNKNOWN_REJECTED\r\n')
 command('tamper','chmod u+w .nova-apps/sqlite; echo broken > .nova-apps/sqlite; if nova-pkg run sqlite :memory:; then echo UNEXPECTED; else echo TAMPER_REJECTED; fi','\r\nTAMPER_REJECTED\r\n')
 command('remove','nova-pkg remove sqlite','sqlite: removed; user databases preserved')
 command('reinstall','nova-pkg install sqlite','sqlite: installed from verified local catalog')
 command('user-data-preserved',"nova-pkg run sqlite -list managed.db 'select v from notes;'",'\r\n123\r\n')
 command('remove-again','nova-pkg remove sqlite; nova-pkg status sqlite','sqlite: not installed')
 command('20-install-run-remove-cycles',"ok=1; for i in $(seq 1 20); do nova-pkg install sqlite >/dev/null && nova-pkg run sqlite -list :memory: 'select 1;' >/dev/null && nova-pkg remove sqlite >/dev/null || ok=0; done; echo CYCLES_$ok",'\r\nCYCLES_1\r\n')
 command('readonly-system','echo forbidden > /bin/forbidden; test ! -e /bin/forbidden && echo READONLY_OK','\r\nREADONLY_OK\r\n')
finally:
 a.close();status=os.waitpid(pid,0)[1]
assert status==0,status
assert len(os.listdir('/proc/self/fd'))==baseline_fds
results['parent-fd-stability']='PASS'
assert (home/'sentinel').read_text()=='KEEP\n'
assert (home/'managed.db').is_file()
assert (home/'script-output.txt').read_text()=='1\n4\n9\n16\n25\n'
assert (home/'demo.lua').is_file()
assert not (home/'.nova-apps/sqlite').exists()
result={'status':'PASS','checks':results,'scope':'real PTY helper, UID12004, private namespaces, bounded seccomp; local catalog install/run/tamper reject/remove/reinstall in WSL; not VM','path':str(out)}
(r/'evidence/r34/multi-runtime-result.json').write_text(json.dumps(result,indent=2));print(result)
