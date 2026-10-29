"""Offline development-image refresh: preserve ONLY the bounded home image.
Input raw export must come from the isolated test VM after normal shutdown.
No VM, firmware, keys, protected disk, System partition or old input is edited.
"""
from pathlib import Path
import sys,struct,shutil,subprocess,hashlib,json,tempfile
old,new,dest=map(Path,sys.argv[1:4]);assert old.is_file() and new.is_file() and not dest.exists()
r=Path(__file__).resolve().parents[1];work=Path(tempfile.mkdtemp(prefix='workspace-migration-',dir=r/'evidence/r25'))
def region(p):
 with p.open('rb') as f:
  f.seek(1024);found=[]
  for i in range(128):
   e=f.read(128)
   if e[56:128].decode('utf-16-le').rstrip('\0')=='NOVA_DATA':found.append(struct.unpack_from('<QQ',e,32))
 assert len(found)==1
 a,b=found[0];assert 0<a<=b and (b+1)*512<=p.stat().st_size
 return a*512,(b-a+1)*512
a,n=region(old);b,m=region(new)
olddata=work/'old-data.ext4';newdata=work/'new-data.ext4';home=work/'home.ext4'
with old.open('rb') as f:f.seek(a);olddata.write_bytes(f.read(n))
with new.open('rb') as f:f.seek(b);newdata.write_bytes(f.read(m))
subprocess.run(['debugfs','-R','dump /Users/nova-workspace.ext4 '+str(home),str(olddata)],check=True,capture_output=True)
assert home.stat().st_size in (16*1024*1024,256*1024*1024)
home.chmod(0o600)
if home.stat().st_size==16*1024*1024:
 grown=work/'grown-home.ext4'
 subprocess.run([sys.executable,str(r/'tools/r35-grow-workspace.py'),str(home),str(grown)],check=True,capture_output=True)
 home=grown
subprocess.run(['e2fsck','-fn',str(home)],check=True,capture_output=True)
blocksize=1024<<struct.unpack_from('<I',newdata.read_bytes(),1048)[0]
subprocess.run(['debugfs','-w','-R','rm /Users/nova-workspace.ext4',str(newdata)],check=True,capture_output=True)
assert struct.unpack_from('<I',newdata.read_bytes(),1036)[0]*blocksize >= home.stat().st_size+16*1024*1024, 'insufficient outer Data reserve'
commands=['write '+str(home)+' /Users/nova-workspace.ext4','set_inode_field /Users/nova-workspace.ext4 uid 0','set_inode_field /Users/nova-workspace.ext4 gid 0','set_inode_field /Users/nova-workspace.ext4 mode 0100600','fallocate /Users/nova-workspace.ext4 0 '+str(home.stat().st_size//blocksize-1)]
script=work/'import.txt';script.write_text('\n'.join(commands)+'\n')
subprocess.run(['debugfs','-w','-f',str(script),str(newdata)],check=True,capture_output=True)
subprocess.run(['e2fsck','-fn',str(newdata)],check=True,capture_output=True)
verify=work/'verify.ext4';subprocess.run(['debugfs','-R','dump /Users/nova-workspace.ext4 '+str(verify),str(newdata)],check=True,capture_output=True)
sha=hashlib.sha256(home.read_bytes()).hexdigest();assert hashlib.sha256(verify.read_bytes()).hexdigest()==sha
shutil.copyfile(new,dest)
with dest.open('r+b') as f:f.seek(b);f.write(newdata.read_bytes())
with new.open('rb') as f,dest.open('rb') as g:assert hashlib.sha256(f.read(b)).digest()==hashlib.sha256(g.read(b)).digest()
result={'status':'PASS','scope':'offline isolated development image migration; not production updater','source':str(old),'destination':str(dest),'workspace_sha256':sha,'workspace_bytes':home.stat().st_size,'new_signed_prefix_unchanged':True,'image_sha256':hashlib.sha256(dest.read_bytes()).hexdigest()}
(work/'results.json').write_text(json.dumps(result,indent=2));(r/'evidence/r25/workspace-migration-result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
