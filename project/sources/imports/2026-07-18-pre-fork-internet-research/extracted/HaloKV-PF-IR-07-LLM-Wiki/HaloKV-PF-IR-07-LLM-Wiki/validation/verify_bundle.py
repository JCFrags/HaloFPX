#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, sys

ROOT = Path(__file__).resolve().parents[1]

def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def read_sum_file(path):
    rows = []
    for line_no, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
        if not line.strip(): continue
        if '  ' not in line:
            raise ValueError(f'{path.name}:{line_no}: invalid format')
        digest, rel = line.split('  ', 1)
        rows.append((digest, rel))
    return rows

def verify_sum_file(rel):
    path = ROOT / rel
    failures = []
    rows = read_sum_file(path)
    for expected, target_rel in rows:
        target = ROOT / target_rel
        if not target.is_file():
            failures.append(f'missing {target_rel}')
            continue
        actual = sha256(target)
        if actual != expected:
            failures.append(f'hash mismatch {target_rel}: {actual} != {expected}')
    return len(rows), failures

all_failures = []
counts = {}
for rel in ['manifests/SHA256SUMS','manifests/SOURCE_SHA256SUMS','manifests/ROOT_SHA256SUMS']:
    try:
        count, failures = verify_sum_file(rel)
        counts[rel] = count
        all_failures.extend(f'{rel}: {x}' for x in failures)
    except Exception as e:
        all_failures.append(f'{rel}: {e}')

try:
    inventory = json.loads((ROOT/'manifests/files.json').read_text(encoding='utf-8'))
    for rec in inventory['records']:
        p = ROOT / rec['path']
        if not p.is_file():
            all_failures.append(f'files.json: missing {rec["path"]}')
        elif p.stat().st_size != rec['size_bytes'] or sha256(p) != rec['sha256']:
            all_failures.append(f'files.json: mismatch {rec["path"]}')
    counts['manifests/files.json'] = len(inventory['records'])
except Exception as e:
    all_failures.append(f'manifests/files.json: {e}')

if all_failures:
    print(f'FAIL: {len(all_failures)} integrity error(s)')
    for failure in all_failures[:200]: print(failure)
    sys.exit(1)
print('PASS: bundle integrity verified')
for k, v in counts.items(): print(f'  {k}: {v} records')
