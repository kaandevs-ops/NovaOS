#!/usr/bin/env python3
"""Storage admission only: does not claim browser persistence or guest acceptance."""
import json, os, pathlib, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
results=[]
with tempfile.TemporaryDirectory(prefix="r54-profile-") as td:
 base=pathlib.Path(td); data=base/"Data"; store=data/"Browser"
 store.mkdir(parents=True);os.chmod(data,0o700);os.chmod(store,0o700)
 binary=base/"validate"
 subprocess.run(["gcc","-O2","-Wall","-Wextra",'-DNOVA_BROWSER_PROFILE_DATA="'+str(data)+'"',str(ROOT/"tests/r54-browser-profile.c"),"-o",str(binary)],check=True)
 image=store/"nova-profile.ext4"
 def check(name,accept):
  r=subprocess.run([str(binary)],capture_output=True,text=True)
  ok=(r.returncode==0)==accept
  results.append(dict(name=name,expected_accept=accept,returncode=r.returncode,passed=ok,output=(r.stdout+r.stderr).strip()))
  if not ok:raise AssertionError(results[-1])
 check("missing image",False)
 image.touch(mode=0o600);os.truncate(image,128*1024*1024)
 check("sparse image",False)
 with image.open("r+b") as f:os.posix_fallocate(f.fileno(),0,128*1024*1024)
 check("bounded allocated image and exclusive lock",True)
 os.chmod(image,0o644);check("world-readable image",False);os.chmod(image,0o600)
 os.chown(image,4,4);check("untrusted image owner",False);os.chown(image,0,0)
 alias=store/"alias";os.link(image,alias);check("hard linked image",False);alias.unlink()
 real=store/"real";image.rename(real);image.symlink_to(real);check("symlink image",False);image.unlink();real.rename(image)
 os.chmod(store,0o777);check("writable ancestor",False);os.chmod(store,0o700)
 os.truncate(image,127*1024*1024);check("wrong capacity",False)
 out=ROOT/"evidence/r54/profile-admission.json";out.parent.mkdir(parents=True,exist_ok=True)
 out.write_text(json.dumps(dict(scope="HOST STORAGE ADMISSION ONLY",results=results),indent=2))
 print(json.dumps(dict(passed=len(results),failed=0,evidence=str(out))))
