#!/usr/bin/env python3
from pathlib import Path
import subprocess,json,os
r=Path(__file__).resolve().parents[1];e=r/'evidence';e.mkdir(exist_ok=True)
checks={
'build-x86_64':['bash','scripts/build-r16-files.sh','x86_64'],
'build-arm64':['bash','scripts/build-r16-files.sh','arm64'],
'integration-build-x86_64':['bash','scripts/build-r16-integration.sh','x86_64'],
'integration-build-arm64':['bash','scripts/build-r16-integration.sh','arm64'],
'catalog-grant-runtime':['bash','tests/r16-trusted-runtime.sh'],
'compositor-runtime':['bash','tests/r16-compositor-runtime.sh'],
'new-security-invariants':['python3','tests/r16-security-invariants.py'],
'ui-bridge-sanitizers':['bash','files/tests/run_tests.sh'],
'core-regression':['bash','tests/r16-core-regression.sh'],
'guest-backend-build':['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I','files/src','-I','src/runtime','files/tests/test_guest_adapter.c','files/src/nova_files_core.c','-o','build/x86_64/r16-files/test-guest-adapter'],
'guest-backend-runtime':['build/x86_64/r16-files/test-guest-adapter'],
'process-transport':['python3','tests/r16-files-process.py'],
'userland-dual-arch-syntax':['bash','tests/r16-userland-syntax.sh'],
'source-syntax':['bash','tests/source-syntax.sh'],
'desktop-seccomp-source':['bash','tests/stage5a-phase17-seccomp-ipc-source.sh'],
'security-safe-suite':['bash','tests/run-stage5a-phase18-step3-r2-safe-suite.sh'],
'frozen-step7-suite':['bash','tests/run-stage5a-phase18-step7-r2-package-suite.sh'],
}
results={}
for name,cmd in checks.items():
 with (e/(name+'.log')).open('w') as log:
  cp=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
 results[name]={'exit_code':cp.returncode,'status':'PASS' if cp.returncode==0 else 'FAIL'}
 print(name,results[name]['status'],flush=True)
(e/'results.json').write_text(json.dumps(results,indent=2))
raise SystemExit(any(x['exit_code'] for x in results.values()))
