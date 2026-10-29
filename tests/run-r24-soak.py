from pathlib import Path
import subprocess,os,shutil,json,fcntl
r=Path(__file__).resolve().parents[1];e=r/'evidence/r24';lock=(e/'validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
with (e/'soak-run.log').open('w') as log:cp=subprocess.run(['python3','tests/run-r17-real-compositor.py'],cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'NOVA_PROFILE_ONLY':'1','NOVA_R22_SOAK_CYCLES':'10000','NOVA_R22_SOAK_OUTPUT':str(e/'soak.json')},timeout=310)
shutil.copy2(e/'real-compositor.log',e/'soak-real-compositor.log');(e/'soak-result.json').write_text(json.dumps({'status':'PASS' if cp.returncode==0 else 'FAIL','exit_code':cp.returncode,'scope':'10000 bounded theme+metadata refresh cycles, real host processes; NOT guest or long-duration certification'},indent=2));raise SystemExit(cp.returncode)
