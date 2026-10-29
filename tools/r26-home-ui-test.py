from pathlib import Path
import tempfile,subprocess,json
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='home-ui-',dir=r/'evidence/r26'));(out/'home').mkdir()
cmd=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(r/'src/runtime'),str(r/'tests/r26-home-ui.c'),str(r/'files/src/nova_files_core.c'),str(r/'files/src/nova_files_linux_host.c'),str(r/'files/src/nova_files_font.c'),str(r/'src/runtime/nova_runtime.c'),str(r/'src/runtime/nova_font.c'),'-o',str(out/'test')]
with (out/'test.log').open('w') as f:
 subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,check=True);subprocess.run([str(out/'test'),str(out/'home')],stdout=f,stderr=subprocess.STDOUT,check=True)
print((out/'test.log').read_text());(r/'evidence/r26/ui-result.json').write_text(json.dumps({'status':'PASS','path':str(out),'scope':'real native UI/model, real host filesystem, ASAN/UBSAN; not guest'},indent=2))
