from pathlib import Path
import subprocess,tempfile,json,os
r=Path(__file__).resolve().parents[1];e=r/'evidence/r43';out=Path(tempfile.mkdtemp(prefix='reader-unit-',dir=e));(out/'home').mkdir()
cmd=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=address,undefined',str(r/'tests/r43-read-model.c'),'-o',str(out/'test')]
subprocess.run(cmd,check=True)
p=subprocess.run([str(out/'test'),str(out/'home')],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},timeout=90)
(out/'test.log').write_text(p.stdout);result={'status':'PASS' if p.returncode==0 else 'FAIL','exit':p.returncode,'directory':str(out),'scope':'actual host UID12004 and Linux DAC,ASAN/UBSAN; not NovaOS guest'};(e/'reader-unit-result.json').write_text(json.dumps(result,indent=2));print(p.stdout);print(result);raise SystemExit(p.returncode)
