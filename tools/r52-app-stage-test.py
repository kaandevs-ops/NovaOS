from pathlib import Path
import importlib.util,json,tempfile,subprocess,hashlib,copy
r=Path(__file__).resolve().parents[1];s=importlib.util.spec_from_file_location('stage',r/'tools/nova-app-stage.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
d=Path(tempfile.mkdtemp(prefix='app-stage-',dir=r/'evidence/r52'));pkg=d/'package';pkg.mkdir();src=d/'start.S';src.write_text('.global _start\n_start: mov $60,%rax; xor %rdi,%rdi; syscall\n');subprocess.run(['clang','-nostdlib','-static','-Wl,-z,noexecstack',str(src),'-o',str(pkg/'program')],check=True)
base={'id':'nova.testapp','name':'Test App','client_id':1,'binary':'program','sha256':hashlib.sha256((pkg/'program').read_bytes()).hexdigest(),'sandbox':'desktop-v1','capabilities':['ui.surface','input.text']};results={}
def write(data):(pkg/'app.json').write_text(json.dumps(data))
write(base);out=d/'staged';assert m.stage(pkg,out,'x86_64')['status']=='STAGED_UNSIGNED';results['static_elf_staging']='PASS'
def reject(name,data,output=None,arch='x86_64'):
 write(data)
 try:m.stage(pkg,output or d/name,arch)
 except (ValueError,OSError,TypeError):results[name]='PASS';return
 raise AssertionError(name)
for key,value,name in [('id','nova.files','reserved_id'),('client_id',4,'reserved_slot'),('sandbox','files-ro-v1','profile_escalation'),('capabilities',['ui.surface','filesystem.write'],'filesystem_capability'),('binary','../program','path_traversal'),('sha256','0'*64,'checksum'),('name','Injected\nexec=/bin/sh','service_injection')]:
 bad=copy.deepcopy(base);bad[key]=value;reject(name,bad)
reject('architecture_mismatch',base,arch='arm64');reject('existing_output',base,output=out)
assert (out/'stage-result.json').exists();results['output_not_replaced']='PASS'
write(base);(pkg/'program').rename(pkg/'realprogram');(pkg/'program').symlink_to('realprogram');reject('symlink_binary',base)
(r/'evidence/r52/app-stage-result.json').write_text(json.dumps({'checks':results,'scope':'offline unsigned staging; no execution/installation or signing'},indent=2));print(json.dumps(results))
