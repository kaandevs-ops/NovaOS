from pathlib import Path
import subprocess,tempfile,shutil,os,json,hashlib
candidate=Path(__file__).resolve().parents[1];r=candidate.parent/'phase19-desktop-r19-premium-design-candidate';assert os.getuid()==0
source=r/'build/x86_64/r17-native';fixture=Path(tempfile.mkdtemp(prefix='real-compositor-root.',dir=candidate/'build/x86_64/r17-native'))
for name in ['System/Apps','System/Services','Data/Nova/Apps','run/nova/app','tmp']:(fixture/name).mkdir(parents=True,exist_ok=True)
for p in (source/'System/Apps').iterdir():shutil.copy2(p,fixture/'System/Apps'/p.name)
for p in (source/'System/Services').iterdir():shutil.copy2(p,fixture/'System/Services'/p.name)
for name in ['a','b','files']:shutil.copy2(source/('launch-'+name),fixture/('launch-'+name))
data=fixture/'Data/Nova/Apps/nova.files';data.mkdir()
for name in ['Documents','Projects','Pictures','Downloads','Çalışmalar']:(data/name).mkdir()
(data/'Documents/Archive').mkdir()
for i in range(20):(data/f'Documents/Report-{i:02d}.txt').write_text('Read-only file fixture\n')
(data/'Documents/Welcome.txt').write_text('NovaOS R17 integration fixture\n')
(data/'Notlar-İstanbul.txt').write_text('Read-only UTF-8 fixture\n')
(data/'Readme.txt').write_text('Read-only application private data. Host test fixture, not real user home.\n')
out=candidate/'evidence/r20/before';out.mkdir(exist_ok=True);gallery=out/'gallery';gallery.mkdir(exist_ok=True)
with (out/'real-compositor.log').open('w') as log:
 cp=subprocess.run([str(candidate/'build/x86_64/r17-native/test-r19-reference'),str(fixture),str(r),str(out/'R19_HOST_RENDER_NOT_VM.ppm'),str(out/'renderer-performance.json'),str(out/'R19_FILES_FOCUSED_HOST_NOT_VM.ppm')],stdout=log,stderr=subprocess.STDOUT,timeout=90,env={**os.environ,"NOVA_GALLERY_DIR":str(gallery)})
(out/'real-compositor-result.json').write_text(json.dumps({'status':'PASS' if cp.returncode==0 else 'FAIL','exit_code':cp.returncode,'scope':'actual freestanding apps + actual compositor/novad functions in private root host fixture; DRM and PID1 boot not run','fixture':str(fixture),'test_binary_sha256':hashlib.sha256((candidate/'build/x86_64/r17-native/test-r19-reference').read_bytes()).hexdigest()},indent=2))
print('R17_PRODUCTION_COMPOSITOR_PROCESS_EXIT',cp.returncode,flush=True)
if cp.returncode:print((out/'real-compositor.log').read_text()[-7000:])
raise SystemExit(cp.returncode)
