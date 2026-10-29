from pathlib import Path
import importlib.util,tempfile,os,subprocess,hashlib,json,fcntl
r=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('grow',r/'tools/r35-grow-workspace.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
e=Path(tempfile.mkdtemp(prefix='storage-growth-',dir=r/'evidence/r35'));os.chmod(e,0o700)
src=e/'old.ext4'
with src.open('xb') as f:f.truncate(mod.OLD)
src.chmod(0o600)
subprocess.run(['mke2fs','-q','-t','ext4','-F','-b','1024','-E','root_owner=12004:12004',str(src)],check=True)
data=e/'notes.bin';data.write_bytes(os.urandom(65536))
def debug(image,cmd,write=False):
 cp=subprocess.run(['debugfs']+(['-w'] if write else [])+['-R',cmd,str(image)],check=True,capture_output=True,text=True)
 return cp.stdout+cp.stderr
debug(src,'set_inode_field / mode 040700',True)
debug(src,'write '+str(data)+' /notes.bin',True)
debug(src,'set_inode_field /notes.bin uid 12004',True)
debug(src,'set_inode_field /notes.bin gid 12004',True)
debug(src,'set_inode_field /notes.bin mode 0100600',True)
before=hashlib.sha256(src.read_bytes()).hexdigest();result={}
def reject(name,source,target):
 try:mod.grow(source,target)
 except (ValueError,OSError,subprocess.CalledProcessError):result[name]='PASS'
 else:raise AssertionError(name)
 assert not target.exists() or name=='existing-destination'
existing=e/'existing';existing.write_bytes(b'KEEP');reject('existing-destination',src,existing);assert existing.read_bytes()==b'KEEP'
link=e/'link';link.symlink_to(src);reject('source-symlink',link,e/'reject-link')
hard=e/'hard';os.link(src,hard);reject('source-hardlink',src,e/'reject-hard');hard.unlink()
src.chmod(0o644);reject('source-public-mode',src,e/'reject-mode');src.chmod(0o600)
with src.open('rb') as f:
 fcntl.flock(f,fcntl.LOCK_EX|fcntl.LOCK_NB);reject('source-locked',src,e/'reject-lock')
wrong=e/'wrong';wrong.write_bytes(b'bad');wrong.chmod(0o600);reject('wrong-size',wrong,e/'reject-size')
corrupt=e/'corrupt';corrupt.write_bytes(bytes(mod.OLD));corrupt.chmod(0o600);reject('corrupt-filesystem',corrupt,e/'reject-corrupt')
assert not list(e.glob('.grow-*'));result['failure-staging-cleanup']='PASS'
dest=e/'grown.ext4';growth=mod.grow(src,dest)
assert hashlib.sha256(src.read_bytes()).hexdigest()==before;result['source-unchanged']='PASS'
assert dest.stat().st_size==mod.NEW and dest.stat().st_blocks*512>=mod.NEW;result['256MiB-fully-allocated']='PASS'
assert dest.stat().st_nlink==1 and dest.stat().st_mode&0o777==0o600;result['output-authority']='PASS'
verify=e/'notes-export.bin';debug(dest,'dump /notes.bin '+str(verify));assert verify.read_bytes()==data.read_bytes();result['file-bytes-preserved']='PASS'
stat=debug(dest,'stat /notes.bin');assert '12004' in stat and '0600' in stat;result['file-metadata-preserved']='PASS'
large=e/'large.bin';large.write_bytes(os.urandom(24*1024*1024));debug(dest,'write '+str(large)+' /large.bin',True)
export=e/'large-export.bin';debug(dest,'dump /large.bin '+str(export));assert hashlib.sha256(export.read_bytes()).digest()==hashlib.sha256(large.read_bytes()).digest();result['real-write-beyond-old-capacity']='PASS'
subprocess.run(['e2fsck','-fn',str(dest)],check=True,capture_output=True);result['filesystem-clean-after-write']='PASS'
assert hashlib.sha256(src.read_bytes()).hexdigest()==before
record={'status':'PASS','checks':result,'migration':growth,'path':str(e),'scope':'offline real ext4 resize and data tests; not guest mount/boot'}
(r/'evidence/r35/storage-growth-results.json').write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2))
