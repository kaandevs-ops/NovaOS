#!/usr/bin/env python3
"""Install only into this candidate's x86_64 build root; no signing or VM ops."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,time
p=Path(__file__).resolve().parents[1]
assert os.environ.get('NOVA_R47_BROWSER_COMPAT')=='1'
assert os.environ.get('NOVA_R47_BROWSER_TEST_NAT')=='1', 'Current networking is isolated VBox NAT test-only; DHCP is pending'
root=Path(sys.argv[1]).resolve();assert root==p/'build/x86_64/rootfs'
stage=json.loads((p/'evidence/r47/runtime-stage.json').read_text());src=Path(stage['runtime']);assert not stage['missing']
assert src.is_relative_to(p/'build/browser-host')
for entry in stage['manifest']:
 f=src/entry['path'].lstrip('/');assert f.is_file() and hashlib.sha256(f.read_bytes()).hexdigest()==entry['sha256'],f
browser=root/'System/Browser';browser.parent.mkdir(parents=True,exist_ok=True)
if browser.exists():browser.rename(p/'build'/('browser-retained-'+str(time.time_ns())))
browser.mkdir()
for name in ['bin','etc','lib','lib64','opt','usr']:shutil.copytree(src/name,browser/name,symlinks=True)
# Signed, fixed test DNS. No host resolv.conf or user environment is packaged.
(browser/'etc/resolv.conf').write_text('nameserver 10.0.2.3\noptions timeout:2 attempts:2\n');(browser/'etc/resolv.conf').chmod(0o444)
(browser/'etc/hosts').write_text('127.0.0.1 localhost\n::1 localhost\n');(browser/'etc/hosts').chmod(0o444)
network=root/'System/Network/etc';network.mkdir(parents=True,exist_ok=True)
marker=network/'test-nat.enabled';marker.write_text('virtualbox-nat-test-v1\n');marker.chmod(0o444)
helper=root/'System/Apps/nova-browser-session';helper.parent.mkdir(parents=True,exist_ok=True)
if helper.exists():helper.rename(p/'build'/('browser-session-retained-'+str(time.time_ns())))
shutil.copyfile(p/'build/browser-host/nova-browser-session',helper);helper.chmod(0o555)
print(json.dumps({'root':str(root),'runtime':str(browser),'scope':'unsigned build staging; guest pending','dns':'isolated VBox NAT test-only','home':'session-only private tmpfs'},indent=2))
