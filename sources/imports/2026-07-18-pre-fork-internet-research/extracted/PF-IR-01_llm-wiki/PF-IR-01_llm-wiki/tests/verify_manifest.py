#!/usr/bin/env python3
from pathlib import Path
import hashlib, sys

root=Path(sys.argv[1] if len(sys.argv)>1 else ".").resolve()
manifest=root/"manifests/files.sha256"
bad=[]
count=0
for line in manifest.read_text().splitlines():
    if not line.strip(): continue
    expected, rel=line.split("  ",1)
    p=root/rel
    if not p.exists():
        bad.append((rel,"missing"))
        continue
    actual=hashlib.sha256(p.read_bytes()).hexdigest()
    count+=1
    if actual!=expected:
        bad.append((rel,actual))
if bad:
    for rel,got in bad:
        print("FAIL",rel,got)
    raise SystemExit(1)
print(f"PASS {count} files")
