from pathlib import Path
import subprocess,json
p=Path(__file__).resolve().parents[1];out=p/'build/r48-native';out.mkdir(exist_ok=True);results={}
for arch,target,cc,browser in [('x86_64','x86_64-linux-gnu','gcc','1'),('arm64','aarch64-linux-gnu','aarch64-linux-gnu-gcc','0')]:
 cmds=[['clang','--target='+target,'-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-pic','-nostdlib','-static','-Os','-Wall','-Wextra','-Werror','-Isrc/runtime','-DNOVA_R47_BROWSER_COMPAT='+browser,'-DNOVA_BUILD_ID="R48-source-test"','src/init/start.S','src/runtime/nova_runtime.c','src/runtime/nova_font.c','src/runtime/nova_textedit.c','src/runtime/nova_widget.c','src/runtime/nova_sha256.c','src/runtime/nova_tcg2.c','src/services/compositor.c','-fuse-ld=lld','-Wl,-e,_start','-Wl,--build-id=none','-Wl,-z,noexecstack','-o',str(out/('nova-compositor-'+arch))],[cc,'-std=c11','-O2','-static','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','src/terminal/nova_terminal_session.c','-o',str(out/('nova-terminal-session-'+arch))]]
 for i,c in enumerate(cmds):
  r=subprocess.run(c,cwd=p,capture_output=True,text=True);(p/f'evidence/r48/build-{arch}-{i}.log').write_text(r.stdout+r.stderr);results[f'{arch}-{i}']='PASS' if r.returncode==0 else 'FAIL';print(arch,i,results[f'{arch}-{i}'],flush=True)
  if r.returncode:print(r.stderr[-2500:]);raise SystemExit(r.returncode)
(p/'evidence/r48/component-builds.json').write_text(json.dumps(results,indent=2))
