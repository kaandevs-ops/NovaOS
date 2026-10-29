from pathlib import Path
import subprocess,json,os
r=Path(__file__).resolve().parents[1];e=r/'evidence/r17';results={}
checks={
 'r16-inherited-regressions':['python3','tests/run-r16.py'],
 'native-all-x86_64':['bash','scripts/build-r17-native.sh','x86_64'],
 'native-all-arm64':['bash','scripts/build-r17-native.sh','arm64'],
 'unsigned-rootfs-x86_64':['python3','scripts/build-r17-unsigned-rootfs.py','x86_64'],
 'unsigned-rootfs-arm64':['python3','scripts/build-r17-unsigned-rootfs.py','arm64'],
 'wm-ui-buffer-metrics-sanitizers':['bash','tests/r17-units.sh'],
 'real-compositor-harness-build':['bash','tests/build-r17-real-compositor.sh'],
}
for name,command in checks.items():
 with (e/(name+'.log')).open('w') as log:
  cp=subprocess.run(command,cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
 results[name]={'status':'PASS' if cp.returncode==0 else 'FAIL','exit_code':cp.returncode}
 print(name,results[name]['status'],flush=True)
(e/'results.json').write_text(json.dumps(results,indent=2))
raise SystemExit(any(v['exit_code'] for v in results.values()))
