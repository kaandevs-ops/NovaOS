#!/usr/bin/env python3
"""Unsigned application-only ext4+verity artifacts. No GPT/ESP/keys/VM."""
from pathlib import Path
import json,subprocess,time,hashlib,shutil,re
p=Path(__file__).resolve().parents[1];r=json.loads((p/'evidence/r47/runtime-stage.json').read_text());root=Path(r['runtime'])
assert not r['missing']
# Mounted /proc or scratch files must never be shipped. Exclude user sessions,
# host DNS and devices. Runtime /etc/resolv.conf is provisioned by the launcher.
b=p/'build/browser-host'/('package-'+str(time.time_ns()));b.mkdir()
image=b/'browser.ext4';tree=b/'browser.verity'
stage=b/'root';shutil.copytree(root,stage,symlinks=True,ignore=lambda d,n:[v for v in n if (Path(d)==root/'home/nova' or Path(d) in [root/'tmp',root/'proc',root/'dev',root/'run'] or (Path(d)==root/'etc' and v in ['resolv.conf','hosts']))])
# All runtime files are trusted build artifacts, never user-owned executable state.
for item in [stage,*stage.rglob('*')]:
 if not item.is_symlink():
  import os
  os.chown(item,0,0)
with image.open('xb') as stream:stream.truncate(512*1024*1024)
subprocess.run(['mkfs.ext4','-q','-F','-b','4096','-m','0','-E','lazy_itable_init=0,lazy_journal_init=0','-d',str(stage),str(image)],check=True)
subprocess.run(['e2fsck','-fn',str(image)],check=True,capture_output=True)
f=subprocess.run(['veritysetup','format',str(image),str(tree)],check=True,capture_output=True,text=True)
m=re.search(r'Root hash:\s*([0-9a-f]+)',f.stdout);assert m;root_hash=m[1]
subprocess.run(['veritysetup','verify',str(image),str(tree),root_hash],check=True,capture_output=True)
# Offline negative check: mutate only a fresh disposable copy of this candidate.
bad=b/'tampered.ext4';shutil.copyfile(image,bad)
with bad.open('r+b') as stream:
 stream.seek(8192);v=stream.read(1);stream.seek(8192);stream.write(bytes([v[0]^1]))
negative=subprocess.run(['veritysetup','verify',str(bad),str(tree),root_hash],capture_output=True,text=True)
assert negative.returncode!=0
record={'scope':'unsigned application package; NOT signed boot disk or guest acceptance','package':str(image),'bytes':image.stat().st_size,'sha256':hashlib.sha256(image.read_bytes()).hexdigest(),'verity_root_hash':root_hash,'verity_verify':'PASS','tamper_rejected':'PASS','trusted_root_hash_in_signed_system':'PENDING','runtime_guest_launch':'PENDING'}
(p/'evidence/r47/runtime-package.json').write_text(json.dumps(record,indent=2));print(json.dumps(record,indent=2))
