from pathlib import Path
import subprocess,os,fcntl,json
r=Path(__file__).resolve().parents[1];e=r/'evidence/r23';lock=(e/'validation.lock').open('a+');fcntl.flock(lock,fcntl.LOCK_EX)
allowed=sorted(os.sched_getaffinity(0));os.sched_setaffinity(0,{allowed[len(allowed)//2]})
p=r/'build/x86_64/r17-native/feature-cost';subprocess.run(['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-I'+str(r/'src/runtime'),str(r/'tests/r23-feature-cost.c'),str(r/'src/runtime/nova_runtime.c'),str(r/'src/runtime/nova_font.c'),'-Wl,--gc-sections','-o',str(p)],check=True)
d=json.loads(subprocess.check_output([str(p)],text=True));(e/'feature-cost.json').write_text(json.dumps(d,indent=2));print(json.dumps(d,indent=2))

p=r/'build/x86_64/r17-native/overview-cost'
subprocess.run(['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-I'+str(r/'src/runtime'),str(r/'tests/r23-overview-cost.c'),str(r/'src/runtime/nova_runtime.c'),'-Wl,--gc-sections','-o',str(p)],check=True)
lines=subprocess.check_output([str(p)],text=True).splitlines();d['overview_1000_roundtrips']=json.loads(next(x for x in lines if x.startswith('{')));(e/'feature-cost.json').write_text(json.dumps(d,indent=2));print(json.dumps(d['overview_1000_roundtrips'],indent=2))
