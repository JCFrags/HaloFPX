#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def sha(p:Path)->str:
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()

errors=[]
raw=json.loads((ROOT/'manifests/raw-manifest.json').read_text())
for e in raw['files']:
    p=ROOT/e['path']
    if not p.is_file(): errors.append(f"missing {e['path']}"); continue
    got=sha(p)
    if got!=e['sha256']: errors.append(f"hash {e['path']} expected {e['sha256']} got {got}")

sums=ROOT/'manifests/SHA256SUMS'
for lineno,line in enumerate(sums.read_text().splitlines(),1):
    if not line.strip(): continue
    expected,rel=line.split('  ',1)
    p=ROOT/rel
    if not p.is_file(): errors.append(f"SHA256SUMS:{lineno} missing {rel}"); continue
    got=sha(p)
    if got!=expected: errors.append(f"SHA256SUMS:{lineno} {rel} expected {expected} got {got}")
if errors:
    print('\n'.join(errors),file=sys.stderr);sys.exit(1)
print(f"OK: {len(raw['files'])} raw captures and {sum(1 for _ in sums.open())} bundle files verified")
