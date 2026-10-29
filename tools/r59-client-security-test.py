from pathlib import Path
import subprocess,json
r=Path(__file__).resolve().parents[1];out=r/'evidence/r59';out.mkdir(parents=True,exist_ok=True)
cmd=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections','-Isrc/runtime','tests/r59-store-client-security.c','src/runtime/nova_runtime.c','-Wl,--gc-sections','-o',str(out/'client-security')]
subprocess.run(cmd,cwd=r,check=True);p=subprocess.run([str(out/'client-security')],capture_output=True,text=True,timeout=20);(out/'client-security.log').write_text(p.stdout+p.stderr);(out/'client-security.json').write_text(json.dumps({'status':'PASS' if p.returncode==0 else 'FAIL','exit':p.returncode,'negative_cases':8,'positive_cases':2},indent=2));print(p.stdout);raise SystemExit(p.returncode)
