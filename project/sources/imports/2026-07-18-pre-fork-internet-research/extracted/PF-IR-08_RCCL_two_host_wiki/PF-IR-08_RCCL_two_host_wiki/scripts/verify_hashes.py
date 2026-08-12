#!/usr/bin/env python3
from pathlib import Path
import hashlib, sys
root = Path(__file__).resolve().parents[1]
manifest = root / 'manifests' / 'SHA256SUMS'
errors = []
for line in manifest.read_text(encoding='utf-8').splitlines():
    if not line.strip(): continue
    expected, rel = line.split('  ', 1)
    p = root / rel
    if not p.is_file():
        errors.append(f'MISSING {rel}')
        continue
    h = hashlib.sha256(p.read_bytes()).hexdigest()
    if h != expected:
        errors.append(f'MISMATCH {rel}: {h} != {expected}')
if errors:
    print('\n'.join(errors)); sys.exit(1)
print(f'OK: verified {sum(1 for x in manifest.read_text().splitlines() if x.strip())} files')
