from pathlib import Path
import hashlib
r=Path(__file__).resolve().parents[1];h=hashlib.sha256()
for base in [r/'src',r/'files/src']:
 for p in sorted(base.rglob('*')):
  if p.is_file():h.update(str(p.relative_to(r)).encode());h.update(p.read_bytes())
print('r23-'+h.hexdigest()[:16])
