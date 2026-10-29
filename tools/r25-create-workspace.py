"""Provision a NEW, fixed-capacity user filesystem; never mounts or touches a VM."""
from pathlib import Path
import subprocess,sys,json
p=Path(sys.argv[1]);assert not p.exists();p.parent.mkdir(parents=True,exist_ok=True)
# Random free blocks avoid sparse import; fallocate on the outer filesystem is
# still mandatory. No signing keys or host user data enter this filesystem.
import os
fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_CLOEXEC,0o600)
with os.fdopen(fd,'wb') as f:
 for _ in range(256):f.write(os.urandom(1024*1024))
subprocess.run(['mke2fs','-q','-t','ext4','-F','-b','1024','-L','NOVA_WORKSPACE','-E','root_owner=12004:12004,nodiscard,lazy_itable_init=0,lazy_journal_init=0',str(p)],check=True)
subprocess.run(['debugfs','-w','-R','set_inode_field / mode 040700',str(p)],check=True,capture_output=True)
subprocess.run(['e2fsck','-fn',str(p)],check=True,capture_output=True)
print(p)
