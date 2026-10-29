from pathlib import Path
import tempfile,subprocess,os,json,fcntl
r=Path(__file__).resolve().parents[1];e=Path(tempfile.mkdtemp(prefix='workspace-authority-',dir=r/'evidence/r35'));data=e/'Data';data.mkdir();users=data/'Users';users.mkdir();image=users/'nova-workspace.ext4'
c=e/'check.c';c.write_text('#define _GNU_SOURCE\n#include <errno.h>\n#include <fcntl.h>\n#include <stdio.h>\n#include <unistd.h>\n#include <sys/stat.h>\n#include <sys/ioctl.h>\n#include <sys/mount.h>\n#include "nova_terminal_workspace.h"\nint main(void){int fd=workspace_open();if(fd>=0){close(fd);return 0;}return fd==-2?2:1;}\n')
subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',f'-DNOVA_WORKSPACE_DATA="{data}"','-I'+str(r/'src/terminal'),str(c),'-o',str(e/'check')],check=True)
results={}
def check(name,want):
 rc=subprocess.run([str(e/'check')]).returncode;assert rc==want,(name,rc,want);results[name]='PASS'
check('missing-is-explicit-session-fallback',2)
with image.open('wb') as f:f.truncate(16*1024*1024)
image.chmod(0o600);check('sparse-image-rejected',1)
fd=os.open(image,os.O_RDWR);os.posix_fallocate(fd,0,16*1024*1024);os.close(fd);check('pinned-fully-allocated-root-owned-image',0)
image.chmod(0o666);check('world-writable-image-rejected',1);image.chmod(0o600)
os.chown(image,12004,12004);check('user-owned-image-rejected',1);os.chown(image,0,0)
link=e/'hardlink';os.link(image,link);check('hardlinked-image-rejected',1);link.unlink()
real=users/'saved';image.rename(real);image.symlink_to(real);check('image-symlink-rejected',1);image.unlink();real.rename(image)
users.chmod(0o777);check('writable-parent-rejected',1);users.chmod(0o755)
users.rename(data/'Actual');users.symlink_to(data/'Actual',target_is_directory=True);check('parent-symlink-rejected',1);users.unlink();(data/'Actual').rename(users)
data.chmod(0o777);check('writable-data-root-rejected',1);data.chmod(0o755)
with image.open('r+b') as held:
 fcntl.flock(held,fcntl.LOCK_EX|fcntl.LOCK_NB);check('concurrent-session-lock-rejected',1)
real=users/'saved';image.rename(real);os.mkfifo(image,0o600);check('fifo-rejected-without-blocking',1);image.unlink();real.rename(image)
check('valid-after-negative-cases',0)
with image.open('r+b') as f:f.truncate(24*1024*1024)
check('unapproved-24MiB-rejected',1)
with image.open('r+b') as f:f.truncate(256*1024*1024)
check('sparse-256MiB-rejected',1)
fd=os.open(image,os.O_RDWR);os.posix_fallocate(fd,0,256*1024*1024);os.close(fd)
check('fully-allocated-256MiB-accepted',0)
with image.open('r+b') as f:f.truncate(512*1024*1024)
check('oversized-512MiB-rejected',1)
(e/'results.json').write_text(json.dumps(results,indent=2));(r/'evidence/r35/workspace-authority-result.json').write_text(json.dumps({'path':str(e),'tests':results,'scope':'real fd/openat authority validation; not guest mount or persistence'},indent=2));print(json.dumps(results,indent=2))
