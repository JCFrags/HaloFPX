#!/usr/bin/env python3
from pathlib import Path
import csv, json, re, sys

ROOT = Path(__file__).resolve().parents[1]
errors = []

# Parse all JSON.
json_files = sorted(ROOT.rglob('*.json'))
for p in json_files:
    try:
        json.loads(p.read_text(encoding='utf-8'))
    except Exception as e:
        errors.append(f'JSON {p.relative_to(ROOT)}: {e}')

# Parse all CSV and require unique headers.
csv_files = sorted(ROOT.rglob('*.csv'))
for p in csv_files:
    try:
        with p.open(encoding='utf-8', newline='') as fh:
            r = csv.reader(fh)
            header = next(r)
            if len(header) != len(set(header)):
                errors.append(f'CSV duplicate header {p.relative_to(ROOT)}')
            list(r)
    except Exception as e:
        errors.append(f'CSV {p.relative_to(ROOT)}: {e}')

claims = json.loads((ROOT/'matrices/claims.json').read_text())
ids = [x['claim_id'] for x in claims]
if len(claims) != 83: errors.append(f'Expected 83 claims, got {len(claims)}')
if len(ids) != len(set(ids)): errors.append('Duplicate claim IDs')
label_re = re.compile(r'^\[CLAIM:PFIR07-C\d{3}\]\[CLASS:[^\]]+\]\[STATUS:[^\]]+\]\[SRC:[^\]]+\]$')
for c in claims:
    if not label_re.match(c['literal_label']):
        errors.append(f'Bad literal label {c["claim_id"]}: {c["literal_label"]}')

sources = json.loads((ROOT/'sources/catalog.json').read_text())
if len(sources) < 20: errors.append(f'Unexpectedly small source catalog: {len(sources)}')
for s in sources:
    if not s['document_revision'] or not s['access_date'] or not s['license']:
        errors.append(f'Incomplete source metadata: {s["source_id"]}')
    lp = ROOT / s['local_path']
    if s['archival_status'].startswith('reference plus'):
        if 'not byte-preserved' not in s['archival_status']:
            errors.append('Systemd archival limitation not explicit')
    elif not lp.exists():
        errors.append(f'Missing local source path: {s["source_id"]} {s["local_path"]}')

expected_counts = {
    'matrices/threat-to-control.json': 18,
    'matrices/residual-risks.json': 25,
    'matrices/minimum-security-profile.json': 30,
    'matrices/human-decisions.json': 6,
    'matrices/key-hierarchy.json': 9,
}
for rel, expected in expected_counts.items():
    got = len(json.loads((ROOT/rel).read_text()))
    if got != expected: errors.append(f'{rel}: expected {expected}, got {got}')

# Every HTML wiki page should retain a title and the invariant should be present.
for p in sorted((ROOT/'wiki').glob('*.html')):
    t = p.read_text(encoding='utf-8')
    if '<h1' not in t: errors.append(f'No h1 in {p.relative_to(ROOT)}')
if 'MISS_RECOMPUTE' not in (ROOT/'wiki/00-decision-record.md').read_text():
    errors.append('Decision record lacks MISS_RECOMPUTE invariant')

if errors:
    print(f'FAIL: {len(errors)} validation error(s)')
    for e in errors: print(e)
    sys.exit(1)
print(f'PASS: parsed {len(json_files)} JSON and {len(csv_files)} CSV files; claims/sources/matrix counts valid')
