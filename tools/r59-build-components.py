from pathlib import Path
import subprocess,json,hashlib
r=Path(__file__).resolve().parents[1];out=r/'evidence/r59/build';out.mkdir(parents=True,exist_ok=True);results={}
for arch,target in [('x86_64','x86_64-linux-gnu'),('arm64','aarch64-linux-gnu')]:
 for name,source,extra in [('compositor','src/services/compositor.c',[]),('files','files/src/nova_files_app.c',['files/src/'+n+'.c' for n in ['nova_files_core','nova_files_ui','nova_files_font','nova_files_surface_bridge','nova_files_memory']])]:
  dest=out/(name+'-'+arch);cmd=['clang','--target='+target,'-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pic','-nostdlib','-static','-Os','-Wall','-Wextra','-Werror','-Isrc/runtime','-Ifiles/src','src/init/start.S']+['src/runtime/'+n+'.c' for n in ['nova_runtime','nova_font','nova_textedit','nova_widget','nova_sha256','nova_tcg2']]+extra+[source,'-fuse-ld=lld','-Wl,-e,_start','-Wl,--build-id=none','-Wl,-z,noexecstack','-o',str(dest)]
  b=subprocess.run(cmd,cwd=r,capture_output=True,text=True);(out/(name+'-'+arch+'.log')).write_text(b.stdout+b.stderr);assert not b.returncode,b.stderr
  elf=subprocess.check_output(['readelf','-l',str(dest)],text=True);assert 'INTERP' not in elf
  results[name+'-'+arch]={'status':'BUILD PASS','sha256':hashlib.sha256(dest.read_bytes()).hexdigest()};print(name,arch,'PASS',flush=True)
(r/'evidence/r59/components.json').write_text(json.dumps(results,indent=2))
