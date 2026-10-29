#!/usr/bin/env python3
"""Bounded source-payload checks; not a guarantee that every secret is detectable."""
from pathlib import Path, PurePosixPath
import hashlib
import io
import json
import re
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SKIP = {'.git', 'build', 'evidence', '__pycache__', 'dist'}
FORBIDDEN = {'.key', '.pem', '.p12', '.pfx', '.vdi', '.vmdk', '.nvram', '.img', '.raw', '.qcow2', '.sav', '.iso', '.pdf', '.pptx'}
KEY = re.compile(rb'^-----BEGIN (?:RSA |EC |OPENSSH |ENCRYPTED )?PRIVATE KEY-----', re.M)
TOKEN = re.compile(rb'\b(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{60,})\b')
PERSONAL = re.compile(rb'(?:/home/' + rb'pc/|/mnt/c/Users/' + rb'pc/|C:\\Users\\' + rb'pc\\|192\.168\.1\.(?:135|171)\b)')
failures = []
checked = 0
expanded = 0

def check(name, data):
    global checked, expanded
    checked += 1
    expanded += len(data)
    if expanded > 256 * 1024 * 1024:
        raise RuntimeError('Expanded payload limit exceeded')
    p = PurePosixPath(name.replace('\\', '/'))
    if p.is_absolute() or '..' in p.parts:
        failures.append((name, 'unsafe archive path'))
    synthetic_pdf = (name == 'tests/fixtures/r44-two-pages.pdf' and hashlib.sha256(data).hexdigest() == '33188b82ad0335f9bc2eb092d75eac5ad179d4ea15fbf801e1fc62c79f1003c6')
    if (p.suffix.lower() in FORBIDDEN and not synthetic_pdf) or p.name.endswith(('.vbox', '.vbox-prev')):
        failures.append((name, 'private/generated/user artifact extension'))
    if KEY.search(data) or TOKEN.search(data):
        failures.append((name, 'credential-like content; value withheld'))
    if PERSONAL.search(data):
        failures.append((name, 'personal machine path or previous target address'))

for path in sorted(ROOT.rglob('*')):
    rel = path.relative_to(ROOT)
    if any(part in SKIP for part in rel.parts) or not path.is_file():
        continue
    if path.is_symlink():
        failures.append((str(rel), 'symlink not allowed'))
        continue
    if path.stat().st_size > 45 * 1024 * 1024:
        failures.append((str(rel), 'file exceeds source publication limit'))
        continue
    data = path.read_bytes()
    check(rel.as_posix(), data)
    if path.name.endswith(('.tar.gz', '.tar.bz2', '.tgz')):
        with tarfile.open(fileobj=io.BytesIO(data)) as archive:
            for member in archive:
                if not member.isfile():
                    continue
                if member.size > 45 * 1024 * 1024:
                    raise RuntimeError('Archive member too large')
                check(rel.as_posix() + '!' + member.name, archive.extractfile(member).read())
    elif path.suffix == '.zip':
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            for member in archive.infolist():
                if member.is_dir():
                    continue
                if member.file_size > 45 * 1024 * 1024:
                    raise RuntimeError('Archive member too large')
                check(rel.as_posix() + '!' + member.filename, archive.read(member))

print(json.dumps({'checked_payloads': checked, 'failures': failures, 'status': 'FAIL' if failures else 'PASS'}, indent=2))
raise SystemExit(bool(failures))
