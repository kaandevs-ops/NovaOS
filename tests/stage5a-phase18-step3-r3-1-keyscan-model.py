#!/usr/bin/env python3
"""R3.1 key scanner regression: self-scan clean; real private-key headers denied.

Host-side model only: no guest, no build artifact, no real secret fixture.
"""
import importlib.util
from pathlib import Path
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / 'scripts/windows/stage5a-phase18-step3-production-validation.py'

spec = importlib.util.spec_from_file_location('nova_step3_r3_1_validator', VALIDATOR)
assert spec is not None and spec.loader is not None
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

# The runtime signatures must remain exactly as strict as in R3.
markers = (b'-----BEGIN ' + b'PRIVATE KEY-----',
           b'-----BEGIN RSA ' + b'PRIVATE KEY-----')
source = VALIDATOR.read_bytes()
assert all(marker not in source for marker in markers), 'self-signature false positive remains'

with tempfile.TemporaryDirectory(prefix='nova-step3-r3-1-keyscan-') as temp:
    root = Path(temp)
    for name in module.TOP_FILES:
        (root / name).write_text('test-only metadata\n')
    for name in module.TOP_DIRS:
        (root / name).mkdir()
    # Build a complete-size source inventory to exercise collect() unchanged.
    for i in range(400):
        (root / 'src' / f'file{i:03}.h').write_text('/* harmless fixture */\n')
    copy = root / 'scripts/windows' / VALIDATOR.name
    copy.parent.mkdir(parents=True)
    shutil.copyfile(VALIDATOR, copy)
    inventory = module.collect(root)
    assert len(inventory) == 404, (len(inventory), 'unexpected source inventory')

    fixture = root / 'src' / 'fake-key.fixture'
    for marker in markers:
        # Deliberately fake, header-only fixture; no private key material.
        fixture.write_bytes(marker + b'\nNOT A REAL KEY\n')
        try:
            module.collect(root)
        except RuntimeError as exc:
            assert 'private key found within source allowlist' in str(exc), str(exc)
        else:
            raise AssertionError('real PEM header was not rejected')
    fixture.write_bytes(b'-----BEGIN PUBLIC KEY-----\nNOT A KEY\n')
    assert len(module.collect(root)) == 405

print('PHASE-18 STEP-3 R3.1 PRIVATE-KEY SELF-SCAN + REAL-HEADER REJECTION: PASS')
