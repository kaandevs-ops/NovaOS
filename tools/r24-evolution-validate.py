from pathlib import Path
import subprocess,json,fcntl,os
r=Path(__file__).resolve().parents[1];e=r/'evidence/r24/evolution-20261021';lock=(r/'evidence/r24/validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
checks=[('units',['bash','tests/r24-units.sh'],{}),('telemetry-bound',['python3','tests/r24-telemetry-budget.py'],{}),('authority-wakeup',['python3','tests/r24-authority-wakeup.py'],{})]
for name in ['r23-overview-server','r23-overview-client','r23-preferences','r22-quick','r22-metadata','r21-blit-equivalence','r21-theme','r21-files-view','r20-navigation','r18-visual-ui','r18-shadow-equivalence','r19-native-design','r19-compositor-design']:checks.append((name,['bash','tests/'+name+'.sh'],{}))
checks += [('three-theme-contrast',['python3','tests/r19-contrast.py'],{}),('principal-runtime',['bash','tests/r16-compositor-runtime.sh'],{}),('principal-source',['bash','tests/stage5a-phase16-peer-principal-source.sh'],{}),('catalog-grant',['runuser','-u','pc','--','bash','tests/r16-trusted-runtime.sh'],{}),('r13-comparison',['python3','tests/r16-security-invariants.py'],{}),('authority-identity',['python3','tools/r24-recovery-security-audit.py'],{}),('kernel-sandbox',['python3','tests/r16-sandbox-runtime.py'],{}),('real-process-gallery',['python3','tests/run-r17-real-compositor.py'],{}),('native-export',['python3','tools/r24-native-export.py'],{}),('native-identity',['python3','tools/r24-verify-native.py'],{}),('bounded-10000-soak',['python3','tests/run-r17-real-compositor.py'],{'NOVA_PROFILE_ONLY':'1','NOVA_R22_SOAK_CYCLES':'10000','NOVA_R22_SOAK_OUTPUT':str(e/'soak.json')})]
results={}
for name,cmd,extra in checks:
 print('START',name,flush=True)
 with (e/('validation-'+name+'.log')).open('w') as log:p=subprocess.run(cmd,cwd=r,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1','NOVA_R24_EVIDENCE_DIR':str(e),**extra})
 results[name]={'status':'PASS' if p.returncode==0 else 'FAIL','exit_code':p.returncode};(e/'validation-results.json').write_text(json.dumps(results,indent=2));print(name,results[name],flush=True)
 if p.returncode:raise SystemExit(p.returncode)
