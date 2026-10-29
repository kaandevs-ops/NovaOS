from pathlib import Path
import subprocess,tempfile,json,os
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='desktop-',dir=r/'evidence/r49'))
helper=out/'session';bundle=r/'build/x86_64/r25-terminal/System/Terminal'
commands=[['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_TERM_ROOT="{out}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',f'-DNOVA_SHARED_WORKSPACE_PARENT="{out}/absent"',f'-DNOVA_WORKSPACE_DATA="{out}/absent-data"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],
 ['clang','-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-ffunction-sections','-fdata-sections',f'-DNOVA_TERM_HELPER="{helper}"','-I'+str(r/'src/runtime'),str(r/'tests/r49-desktop-ui.c'),str(r/'src/runtime/nova_runtime.c'),str(r/'src/runtime/nova_font.c'),'-Wl,--gc-sections','-o',str(out/'desktop')],
 [str(out/'desktop'),str(out)]]
with (out/'test.log').open('w') as log:
 for cmd in commands:
  x=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,timeout=180)
  if x.returncode:
   print((out/'test.log').read_text(errors='replace')[-6000:]);raise SystemExit(x.returncode)
(r/'evidence/r49/desktop-ui-result.json').write_text(json.dumps({'status':'PASS','scope':'production compositor functions + real shell/PTY in WSL; not DRM/VM','path':str(out)},indent=2))
print((out/'test.log').read_text()[-1500:]);print(out)
