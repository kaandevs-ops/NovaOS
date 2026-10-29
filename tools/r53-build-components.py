from pathlib import Path
import subprocess,json
r=Path(__file__).resolve().parents[1];out=r/'build/r53-components';out.mkdir(parents=True,exist_ok=True);results={}
runtime=['src/init/start.S','src/runtime/nova_runtime.c','src/runtime/nova_sha256.c','src/runtime/nova_tpm2.c','src/runtime/nova_tcg2.c','src/runtime/nova_font.c','src/runtime/nova_textedit.c','src/runtime/nova_widget.c']
with (r/'evidence/r53/component-build.log').open('w') as log:
 for arch,target,gcc in [('x86_64','x86_64-linux-gnu','gcc'),('arm64','aarch64-linux-gnu','aarch64-linux-gnu-gcc')]:
  for name,source in [('compositor','src/services/compositor.c'),('shell','src/services/shell.c'),('novad','src/init/main.c')]:
   cmd=['clang','--target='+target,'-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pic','-ffunction-sections','-fdata-sections','-nostdlib','-static','-Os','-Wall','-Wextra','-Werror','-Isrc/runtime',*runtime,source,'-fuse-ld=lld','-Wl,--gc-sections','-Wl,-e,_start','-Wl,-z,noexecstack','-o',str(out/(arch+'-'+name))]
   subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,check=True);results[arch+'-'+name]='BUILD PASS'
  subprocess.run([gcc,'-std=c11','-Os','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','src/compat/nova_browser_session.c','-o',str(out/(arch+'-browser-bootstrap'))],cwd=r,stdout=log,stderr=subprocess.STDOUT,check=True);results[arch+'-browser-bootstrap']='BUILD PASS, not browser runtime/guest acceptance'
(r/'evidence/r53/component-build-result.json').write_text(json.dumps(results,indent=2));print(json.dumps(results))
