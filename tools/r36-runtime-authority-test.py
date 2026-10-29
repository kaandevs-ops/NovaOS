from pathlib import Path
import subprocess,tempfile,os,socket,struct,json,shutil
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='runtime-authority-',dir=r/'evidence/r36'));bundle=out/'bundle'
shutil.copytree(r/'build/x86_64/r25-terminal/System/Terminal',bundle,symlinks=True)
helper=out/'helper'
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_SHARED_WORKSPACE_PARENT="{out}/absent"',f'-DNOVA_WORKSPACE_DATA="{out}/absent-data"',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],check=True)
results={};baseline=len(os.listdir('/proc/self/fd'))
def check(name,ready=False):
 a,b=socket.socketpair(socket.AF_UNIX,socket.SOCK_SEQPACKET);a.settimeout(15)
 actions=[(os.POSIX_SPAWN_CLOSE,a.fileno()),(os.POSIX_SPAWN_DUP2,b.fileno(),3)]
 if b.fileno()!=3:actions.append((os.POSIX_SPAWN_CLOSE,b.fileno()))
 pid=os.posix_spawn(str(helper),[str(helper)],{},file_actions=actions);b.close()
 try:
  data=a.recv(4096);h=struct.unpack('<8I',data[:32]);assert h[0]==0x4e54524d and h[2]==(16 if ready else 19),(name,h)
 finally:a.close();os.waitpid(pid,0)
 results[name]='PASS'
rt=bundle/'Runtime';lib=rt/'lib';obj=lib/'libc.so.6'
check('valid-readonly-runtime',True)
rt.chmod(0o777);check('writable-runtime-rejected');rt.chmod(0o755)
os.chown(rt,12004,12004);check('user-owned-runtime-rejected');os.chown(rt,0,0)
lib.rename(rt/'lib-saved');lib.symlink_to('lib-saved',target_is_directory=True);check('library-directory-symlink-rejected');lib.unlink();(rt/'lib-saved').rename(lib)
obj.chmod(0o666);check('writable-library-rejected');obj.chmod(0o444)
os.chown(obj,12004,12004);check('user-owned-library-rejected');os.chown(obj,0,0)
obj.rename(lib/'libc-saved');obj.symlink_to('libc-saved');check('library-symlink-rejected');obj.unlink();(lib/'libc-saved').rename(obj)
os.link(obj,lib/'extra-link');check('library-hardlink-rejected');(lib/'extra-link').unlink()
(rt/'lib64').rename(rt/'lib64-saved');check('missing-runtime-directory-rejected');(rt/'lib64-saved').rename(rt/'lib64')
rt.rename(bundle/'Runtime-saved');check('legacy-static-bundle-still-starts',True);(bundle/'Runtime-saved').rename(rt)
check('valid-after-negative-cases',True)
assert len(os.listdir('/proc/self/fd'))==baseline;results['parent-fd-stability']='PASS'
record={'status':'PASS','checks':results,'path':str(out),'scope':'real production launcher, namespaces, owner/mode/link rejection and legacy compatibility in WSL; not VM'}
(r/'evidence/r36/runtime-authority-result.json').write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2))
