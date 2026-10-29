#!/usr/bin/env python3
from pathlib import Path
import sys,uuid
root=Path(__file__).resolve().parents[1];allowed=(root/'build').resolve();archive=allowed/'retained';archive.mkdir(exist_ok=True)
for arg in sys.argv[1:]:
 if arg.startswith('-'):continue
 p=Path(arg);p=p if p.is_absolute() else Path.cwd()/p
 if not p.exists() and not p.is_symlink():continue
 resolved=p.resolve();assert resolved.is_relative_to(allowed) and resolved!=allowed,(str(p),str(resolved))
 p.rename(archive/(uuid.uuid4().hex+'-'+p.name))
