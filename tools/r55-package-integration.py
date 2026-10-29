#!/usr/bin/env python3
"""Exercise real commands in production isolated PTY helper, not host shell."""
from pathlib import Path
import subprocess,tempfile,os,socket,struct,json,time
r=Path(__file__).resolve().parents[1]
out=Path(tempfile.mkdtemp(prefix='package-lifecycle-',dir=r/'evidence/r55'))
parent=out/'user-session';parent.mkdir(mode=0o700)
home=parent/'home';home.mkdir(mode=0o700);os.chown(home,12004,12004)
helper=out/'session';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
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
 command('catalog','nova-pkg list','sqlite 3.54.1')
 command('not-installed','nova-pkg status sqlite','sqlite: not installed')
 command('run-before-install',"if nova-pkg run sqlite :memory: 'select 1;'; then echo UNEXPECTED; else echo NO_INSTALL_REJECTED; fi",'\r\nNO_INSTALL_REJECTED\r\n')
 command('install','nova-pkg install sqlite','sqlite: installed from verified local catalog')
 command('verified-status','nova-pkg status sqlite','sqlite: installed, verified')
 command('run',"nova-pkg run sqlite -batch -noheader -list :memory: 'select sqlite_version();'",'\r\n3.54.1\r\n')
 command('database',"nova-pkg run sqlite -list managed.db \"create table notes(v); insert into notes values(123); select v from notes;\"",'\r\n123\r\n')
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
 command('catalog-json','nova-pkg list --json > catalog.json; echo JSON_STATUS_$?','JSON_STATUS_0')
 command('metadata-json','nova-pkg info 2048 --json > game-info.json; echo INFO_STATUS_$?','INFO_STATUS_0')
 command('search-games','nova-pkg search GAMES','2048 1.0.3')
 command('search-empty-result','nova-pkg search NO_SUCH_GAME_999','No matching package')
 command('invalid-info-id','nova-pkg info ../../etc; echo INVALID_STATUS_$?','INVALID_STATUS_2')
 command('game-install','nova-pkg install 2048','2048: installed from verified local catalog')
 a.send(struct.pack('<8I',0x4e54524d,1,2,0,10,40,0,0));time.sleep(.1)
 command('game-small-window','nova-pkg run 2048; echo GAME_SIZE_$?','GAME_SIZE_2')
 a.send(struct.pack('<8I',0x4e54524d,1,2,0,24,80,0,0));time.sleep(.1)
 command('game-installed-list','nova-pkg installed','2048 1.0.3 - installed, verified')
 command('game-version','nova-pkg run 2048 --version','2048.c version 1.0.3')
 command('game-algorithm','nova-pkg run 2048 test','All 13 tests executed successfully')
 cmd='nova-pkg run 2048';a.send(struct.pack('<8I',0x4e54524d,1,1,len(cmd)+1,0,0,0,0)+cmd.encode()+b'\r')
 data=b''
 while b'2048.c' not in data:
  h,v=receive();assert h[2]==17;data+=v
 for keys in [b'\x1b[C',b'\x1b[B',b'q',b'y']:
  a.send(struct.pack('<8I',0x4e54524d,1,1,len(keys),0,0,0,0)+keys);time.sleep(.25)
 data+=prompt();(out/'game-interaction.log').write_bytes(data)
 assert b'QUIT?' in data and data.count(b'2048.c')>=2,data[-3000:]
 results['real-game-pty-arrows-quit']='PASS'
 command('game-shell-survives','pwd','/home/nova')
 command('game-tamper','chmod u+w .nova-apps/2048; printf CORRUPT > .nova-apps/2048; nova-pkg installed; echo INVALID_STATE_$?','INVALID_STATE_1')
 command('game-tamper-run','nova-pkg run 2048; echo INVALID_RUN_$?','INVALID_RUN_1')
 command('game-remove','nova-pkg remove 2048; nova-pkg status 2048','2048: not installed')
 command('game-repeat','ok=1; for n in $(seq 1 20); do nova-pkg install 2048 >/dev/null && nova-pkg run 2048 test >/dev/null && nova-pkg remove 2048 >/dev/null || ok=0; done; echo GAME_CYCLES_$ok','GAME_CYCLES_1')
 command('software-tty-before','before_modes=$(stty -g); echo SAVED_TTY','SAVED_TTY')
 def send(raw):a.send(struct.pack('<8I',0x4e54524d,1,1,len(raw),0,0,0,0)+raw)
 def until(marker):
  outdata=b''
  while marker not in outdata:
   h,v=receive();assert h[2]==17;outdata+=v
  return outdata
 menu_end=b'mevcut sandbox ile calisir.'
 send(b'nova-software\r');until(menu_end);send(b'4');until(menu_end)
 a.send(struct.pack('<8I',0x4e54524d,1,2,0,10,40,0,0));until(b'window too small.')
 a.send(struct.pack('<8I',0x4e54524d,1,2,0,24,80,0,0));until(menu_end)
 results['software-live-resize-warning-restore']='PASS'
 send(b'i');installed=until(b'Bir tusa basin...');assert b'2048: installed from verified local catalog' in installed
 send(b' ');until(menu_end);send(b'o');played=until(b'2048.c');send(b'\x1b[C');time.sleep(.2);send(b'qy');played+=until(menu_end);assert b'QUIT?' in played
 send(b's');state=until(b'Bir tusa basin...');assert b'installed, verified' in state
 send(b' ');until(menu_end);send(b'r');until(b'diger: iptal');send(b'n');until(menu_end)
 send(b'r');until(b'diger: iptal');send(b'y');removed=until(b'Bir tusa basin...');assert b'2048: removed' in removed
 send(b' ');until(menu_end);send(b'q');closed=prompt();assert b'Nova Software kapandi.' in closed
 (out/'software-interaction.log').write_bytes(installed+played+state+removed+closed)
 results['software-real-menu-install-play-status-cancel-confirm-remove']='PASS'
 command('software-tty-restored','test "$before_modes" = "$(stty -g)" && echo TTY_RESTORED','TTY_RESTORED')
finally:
 a.close();status=os.waitpid(pid,0)[1]
assert status==0,status
assert len(os.listdir('/proc/self/fd'))==baseline_fds
results['parent-fd-stability']='PASS'
assert (home/'managed.db').is_file()
assert (home/'script-output.txt').read_text()=='1\n4\n9\n16\n25\n'
assert (home/'demo.lua').is_file()
assert not (home/'.nova-apps/sqlite').exists()
catalog=json.loads((home/'catalog.json').read_text());assert len(catalog)==4 and {x['id'] for x in catalog}=={'sqlite','lua','lua-dynamic','2048'};assert all(x['interface']=='terminal' and x['network_install'] is False for x in catalog);info=json.loads((home/'game-info.json').read_text());assert info['category']=='games' and info['version']=='1.0.3'
result={'status':'PASS','checks':results,'scope':'real PTY helper, UID12004, private namespaces, bounded seccomp; local catalog install/run/tamper reject/remove/reinstall in WSL; not VM','path':str(out)}
(r/'evidence/r55/applications-result.json').write_text(json.dumps(result,indent=2));print(result)
