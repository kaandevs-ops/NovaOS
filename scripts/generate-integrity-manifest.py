#!/usr/bin/env python3
import hashlib
import os
import sys

root = os.path.abspath(sys.argv[1])
out_path = os.path.join(root, "System", "Integrity", "manifest.sha256")
os.makedirs(os.path.dirname(out_path), exist_ok=True)

entries = []
for top in ("System", "Recovery"):
    base = os.path.join(root, top)
    for dirpath, _, filenames in os.walk(base):
        for name in filenames:
            path = os.path.join(dirpath, name)
            guest = "/" + os.path.relpath(path, root).replace(os.sep, "/")
            if guest in ("/System/Integrity/manifest.sha256", "/System/Integrity/manifest.sig"):
                continue
            with open(path, "rb") as f:
                digest = hashlib.sha256(f.read()).hexdigest()
            entries.append((guest, digest))
entries.sort()
with open(out_path, "w", encoding="utf-8", newline="\n") as f:
    f.write("# NovaOS Stage-4C integrity manifest\n")
    f.write("# SHA-256 content hashes authenticated by manifest.sig.\n")
    for guest, digest in entries:
        f.write(f"{digest}  {guest}\n")
print(f"Generated: {out_path} ({len(entries)} entries)")
