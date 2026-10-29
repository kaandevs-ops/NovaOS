from pathlib import Path
import hashlib,json,shutil,sys
r=Path(__file__).resolve().parents[1];dest=Path(sys.argv[1]);sources=[r/'assets/webapps'/name for name in ['tetris','music']]
if dest.exists():raise SystemExit('Refusing to replace an existing web application staging directory')
for src in sources:
 m=json.loads((src/'nova-webapp.json').read_text())
 for name,digest in m['files'].items():
  p=src/name
  if Path(name).name!=name or p.is_symlink() or hashlib.sha256(p.read_bytes()).hexdigest()!=digest:raise SystemExit('Invalid web application asset: '+name)
dest.mkdir(parents=True)
for src in sources:
 shutil.copytree(src,dest/src.name)
 for p in (dest/src.name).rglob('*'):p.chmod(0o555 if p.is_dir() else 0o444)
print('Offline Tetris and Music staged; real NovaOS guest interaction/audio PENDING')
