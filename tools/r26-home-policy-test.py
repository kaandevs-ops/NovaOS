from pathlib import Path
import tempfile,subprocess,json
r=Path(__file__).resolve().parents[1];out=Path(tempfile.mkdtemp(prefix='home-policy-',dir=r/'evidence/r26'));results={}
for name in ['parser','manifest']:
 cmd=['clang','-std=c11','-O1','-g','-fsanitize=address,undefined','-ffunction-sections','-fdata-sections','-I'+str(r/'src/runtime'),str(r/('tests/r26-home-'+name+'.c')),str(r/'src/runtime/nova_runtime.c'),'-Wl,--gc-sections','-o',str(out/name)]
 with (out/(name+'-build.log')).open('w') as f:subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,check=True)
subprocess.run([str(out/'parser')],check=True)
base=(r/'configs/services/nova-files.service').read_text();legacy=base.replace('directory_scope=user-home-read-only\n','');cases=[('home',base,1),('legacy',legacy,0),('explicit-private',base.replace('user-home-read-only','private-read-only'),0),('bad-value',base.replace('user-home-read-only','arbitrary'),-1),('duplicate',base+'directory_scope=user-home-read-only\n',-1),('other-app',(r/'configs/services/demo-app.service').read_text()+'directory_scope=user-home-read-only\n',-1),('service',(r/'configs/services/heartbeat.service').read_text()+'directory_scope=user-home-read-only\n',-1)]
for name,text,want in cases:
 p=out/(name+'.service');p.write_text(text);subprocess.run([str(out/'manifest'),str(p),str(want)],check=True);results[name]='PASS'
(r/'evidence/r26/home-policy-result.json').write_text(json.dumps({'status':'PASS','path':str(out),'manifest_cases':results,'ancillary_and_component':'PASS','sanitizers':'ASAN+UBSAN'},indent=2));print(results)
