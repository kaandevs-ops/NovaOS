#!/usr/bin/env python3
"""Fetch pinned upstream source only; does not extract, execute or sign."""
from pathlib import Path
import hashlib,json,urllib.request,os,tempfile
r=Path(__file__).resolve().parents[1]
p=r/'third_party/mupdf/SOURCE.json';m=json.loads(p.read_text())
assert m['url'].startswith('https://casper.mupdf.com/downloads/archive/')
out=p.parent/('mupdf-'+m['version']+'-source.tar.gz')
if out.exists():
 assert hashlib.sha256(out.read_bytes()).hexdigest()==m['sha256'];print('Already verified');raise SystemExit
fd,tmp=tempfile.mkstemp(prefix='.download-',dir=p.parent)
try:
 with os.fdopen(fd,'wb') as w,urllib.request.urlopen(m['url'],timeout=60) as response:
  if not response.url.startswith('https://'):raise RuntimeError('HTTPS required')
  total=0
  while chunk:=response.read(1024*1024):
   total+=len(chunk)
   if total>150*1024*1024:raise RuntimeError('Source size limit')
   w.write(chunk)
 assert hashlib.sha256(Path(tmp).read_bytes()).hexdigest()==m['sha256'],'Checksum mismatch'
 os.link(tmp,out)
 print('Upstream source hash verified')
finally:
 Path(tmp).unlink(missing_ok=True)
