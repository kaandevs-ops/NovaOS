#!/usr/bin/env python3
from pathlib import Path
import subprocess,tempfile,json
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'evidence/r54';OUT.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='r54-desktop-files-') as td:
 binary=Path(td)/'test';home=Path(td)/'home';home.mkdir(mode=0o700)
 # User12004 must traverse only this fixture directory, no system grants.
 Path(td).chmod(0o755)
 subprocess.run(['gcc','-std=c11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function','-I'+str(ROOT/'src/terminal'),str(ROOT/'tests/r54-desktop-files.c'),'-o',str(binary)],check=True)
 run=subprocess.run([str(binary),str(home)],capture_output=True,text=True,timeout=90)
 result=dict(scope='HOST REAL FILESYSTEM/UID12004; NOT GUEST',passed=run.returncode==0,stdout=run.stdout,stderr=run.stderr)
 (OUT/'desktop-files-storage.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
 if run.returncode:raise SystemExit(run.returncode)
