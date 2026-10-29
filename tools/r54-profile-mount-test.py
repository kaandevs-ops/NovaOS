#!/usr/bin/env python3
import pathlib, tempfile, subprocess, os, json
ROOT=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="r54-mount-") as td:
 base=pathlib.Path(td);data=base/"Data";store=data/"Browser";store.mkdir(parents=True)
 os.chmod(data,0o700);os.chmod(store,0o700)
 image=store/"nova-profile.ext4"
 with image.open("wb") as f:os.posix_fallocate(f.fileno(),0,128*1024*1024)
 os.chmod(image,0o600)
 subprocess.run(["mke2fs","-q","-t","ext4","-F",str(image)],check=True)
 # mke2fs may punch holes; production provisioning must fully allocate after mkfs.
 with image.open("r+b") as f:os.posix_fallocate(f.fileno(),0,128*1024*1024)
 target=base/"mount";target.mkdir();binary=base/"mount-test"
 subprocess.run(["gcc","-Wall","-Wextra","-Werror","-O2",'-DNOVA_BROWSER_PROFILE_DATA="'+str(data)+'"',str(ROOT/"tests/r54-profile-mount.c"),"-o",str(binary)],check=True)
 r=subprocess.run(["unshare","--mount","--propagation","private",str(binary),str(target)],text=True,capture_output=True)
 evidence=dict(scope="HOST EXT4 STORAGE ONLY; NOT FIREFOX OR GUEST",passed=r.returncode==0,returncode=r.returncode,stdout=r.stdout,stderr=r.stderr)
 out=ROOT/"evidence/r54/profile-mount.json";out.write_text(json.dumps(evidence,indent=2));print(json.dumps(evidence))
 if r.returncode:raise SystemExit(r.returncode)
