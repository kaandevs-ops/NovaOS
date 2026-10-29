from pathlib import Path
import subprocess,json,hashlib
r=Path(__file__).resolve().parents[1];out=r/'build/r46';out.mkdir(exist_ok=True);results={};build_id=subprocess.check_output(['python3',str(r/'tools/r24-build-id.py')],text=True).strip()
for arch,target in [('x86_64','x86_64-linux-gnu'),('arm64','aarch64-linux-gnu')]:
 cmd=['clang','--target='+target,'-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pic','-nostdlib','-static','-Os','-Wall','-Wextra','-Werror','-DNOVA_BUILD_ID="'+build_id+'"','-Isrc/runtime','src/init/start.S']+[f'src/runtime/{n}.c' for n in ['nova_runtime','nova_font','nova_textedit','nova_widget','nova_sha256','nova_tcg2']]+['src/services/compositor.c','-fuse-ld=lld','-Wl,-e,_start','-Wl,--build-id=none','-Wl,-z,noexecstack','-o',str(out/('nova-compositor-'+arch))]
 q=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(r/f'evidence/r46/build-{arch}.log').write_text(q.stdout+q.stderr);assert q.returncode==0,q.stderr
 f=out/('nova-compositor-'+arch);results[arch]={'status':'BUILD_PASS','sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'bytes':f.stat().st_size}
for arch,target in [('x86_64','x86_64-linux-gnu'),('arm64','aarch64-linux-gnu')]:
 cmd=['clang','--target='+target,'-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pic','-nostdlib','-static','-Os','-Wall','-Wextra','-Werror','-DNOVA_BUILD_ID=\"'+build_id+'\"','-Isrc/runtime','src/init/start.S','src/runtime/nova_runtime.c','src/runtime/nova_sha256.c','src/runtime/nova_tcg2.c','src/runtime/nova_tpm2.c','src/init/main.c','-fuse-ld=lld','-Wl,-e,_start','-Wl,--build-id=none','-Wl,-z,noexecstack','-o',str(out/('novad-'+arch))]
 q=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(r/f'evidence/r46/build-novad-{arch}.log').write_text(q.stdout+q.stderr);assert q.returncode==0,q.stderr
 f=out/('novad-'+arch);results['novad_'+arch]={'status':'BUILD_PASS','sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'bytes':f.stat().st_size}
results['scope']='compositor and novad static ELF only; not full rootfs or VM; TLS transport host test separate';results['build_id']=build_id
(r/'evidence/r46/native-build-results.json').write_text(json.dumps(results,indent=2));print(json.dumps(results,indent=2))
