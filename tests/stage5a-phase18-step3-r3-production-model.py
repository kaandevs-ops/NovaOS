#!/usr/bin/env python3
"""Source-level R3 normal-build fail-closed validation; no guest claims."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
SOURCE_SCRIPTS=['build-userland.sh','build-initramfs.sh']
DENIED_ARTIFACTS=[
    'System/Apps/nova-security-probe-read',
    'System/Apps/nova-sandbox-probe-a',
    'System/Apps/nova-resource-probe-grant',
    'System/Services/phase16-security-probe-read.service',
    'System/Services/phase17-sandbox-probe-a.service',
    'System/Services/phase18-resource-probe-grant.service',
]
with tempfile.TemporaryDirectory(prefix='nova-step3-r3-') as temp:
    p=Path(temp)
    scripts=p/'scripts'
    scripts.mkdir()
    for name in SOURCE_SCRIPTS:
        shutil.copy2(ROOT/'scripts'/name,scripts/name)
    rootfs=p/'build/x86_64/rootfs'
    broker=rootfs/'System/Core/nova-resource-broker'
    env=os.environ.copy()
    env.update(NOVA_PHASE16_ACCEPTANCE_PROBES='0', NOVA_PHASE17_ACCEPTANCE_PROBES='0',
               NOVA_PHASE18_ACCEPTANCE_PROBES='0', NOVA_PHASE18_STEP3_ADVERSARIAL='0')
    for artifact in DENIED_ARTIFACTS:
        target=rootfs/artifact
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_text('PRESERVE-ME')
        for script in SOURCE_SCRIPTS:
            result=subprocess.run(['bash',str(scripts/script),'x86_64'],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            assert result.returncode==2,(script,artifact,result.stdout)
            assert 'FAIL-CLOSED' in result.stdout,(script,artifact,result.stdout)
            assert target.read_text()=='PRESERVE-ME','guard modified existing file'
        target.unlink()
    broker.parent.mkdir(parents=True,exist_ok=True)
    broker.write_bytes(b'ELF HEADER '+b'step3_fixture_mode=adversarial-only')
    for script in SOURCE_SCRIPTS:
        result=subprocess.run(['bash',str(scripts/script),'x86_64'],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        assert result.returncode==2,(script,result.stdout)
        assert 'FAIL-CLOSED' in result.stdout,(script,result.stdout)
    assert b'adversarial-only' in broker.read_bytes()
print('Phase-18 Step-3 R3 normal-build fail-closed artifact + broker-marker model: PASS')
