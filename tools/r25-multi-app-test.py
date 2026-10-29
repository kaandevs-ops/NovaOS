from pathlib import Path
import tempfile,subprocess,os,json
r=Path(__file__).resolve().parents[1];e=Path(tempfile.mkdtemp(prefix='multi-',dir=r/'evidence/r25'));helper=e/'session';bundle=r/'build/x86_64/r25-terminal/System/Terminal';binary=e/'test-real-compositor'
commands=[['gcc','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',f'-DNOVA_TERM_ROOT="{e}/root"',f'-DNOVA_TERM_BUNDLE="{bundle}"',str(r/'src/terminal/nova_terminal_session.c'),'-o',str(helper)],
 ['clang','-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',f'-DNOVA_TERM_HELPER="{helper}"','-I'+str(r/'src/runtime'),'-I'+str(r/'files/src'),str(r/'tests/r17-real-compositor.c'),str(r/'tests/r17-fixture-authority.c'),*[str(r/x) for x in ['files/src/nova_files_core.c','files/src/nova_files_ui.c','files/src/nova_files_font.c','src/runtime/nova_runtime.c','src/runtime/nova_font.c']],'-Wl,--gc-sections','-o',str(binary)],
 ['python3','tests/run-r17-real-compositor.py']]
with (e/'test.log').open('w') as log:
 for cmd in commands:
  p=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,timeout=180,env={**os.environ,'NOVA_R24_EVIDENCE_DIR':str(e),'NOVA_R24_COMPOSITOR_TEST_BINARY':str(binary),'NOVA_PROFILE_ONLY':'1','NOVA_R25_TERMINAL_TEST':'1','NOVA_R25_MULTI_OUT':str(e/'three-apps-terminal.HOST-NOT-VM.ppm')})
  if p.returncode:print((e/'test.log').read_text()[-5000:]);raise SystemExit(p.returncode)
(r/'evidence/r25/multi-result.json').write_text(json.dumps({'status':'PASS','path':str(e),'scope':'three real freestanding apps + real terminal helper; host compositor functions; supervisor authority fixture, not PID1 boot'},indent=2));print(e)
