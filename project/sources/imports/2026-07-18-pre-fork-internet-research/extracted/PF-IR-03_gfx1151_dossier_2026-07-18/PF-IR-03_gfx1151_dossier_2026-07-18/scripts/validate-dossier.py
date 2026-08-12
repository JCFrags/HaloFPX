#!/usr/bin/env python3
from __future__ import annotations
import csv, hashlib, json, pathlib, sys
import yaml
root = pathlib.Path(__file__).resolve().parents[1]
errors=[]
for p in sorted(root.rglob('*.json')):
    try: json.loads(p.read_text(encoding='utf-8'))
    except Exception as e: errors.append(f'{p.relative_to(root)}: JSON: {e}')
for p in sorted(root.rglob('*.yaml')):
    try: yaml.safe_load(p.read_text(encoding='utf-8'))
    except Exception as e: errors.append(f'{p.relative_to(root)}: YAML: {e}')
for p in sorted(root.rglob('*.csv')):
    try:
        with p.open(encoding='utf-8', newline='') as f: list(csv.DictReader(f))
    except Exception as e: errors.append(f'{p.relative_to(root)}: CSV: {e}')
required = [
 'index.html','README.md','manifests/claims.json','manifests/artifacts.lock.yaml',
 'manifests/source-pins.yaml','manifests/open-base-01-external.yaml',
 'docs/00-executive-decision.md','raw/provenance-gaps/7.14-tarball-integrity.md'
]
for r in required:
    if not (root/r).is_file(): errors.append(f'missing {r}')
claims=(root/'manifests/claims.json').read_text(encoding='utf-8')
for label in ['[DOCUMENTED_SUPPORT]','[PREVIEW_AVAILABILITY]','[KNOWN_ISSUE]','[UNVERIFIED_COMBINATION]','[LOCAL_COMPARISON_ONLY]','[PROVENANCE_GAP]','[DO_NOT_MIX]']:
    if label not in claims: errors.append(f'missing claim label {label}')
lock=yaml.safe_load((root/'manifests/artifacts.lock.yaml').read_text())
tar=next(x for x in lock['artifacts'] if x['id']=='rocm-7.14.0-linux-gfx1151-tarball')
if tar.get('sha256') is not None or tar.get('promotable') is not False:
    errors.append('7.14 tarball must retain null sha256 and promotable=false')
for distro in ('ubuntu2404','ubuntu2604'):
    expected=f'rocm-7.14.0-{distro}-native-packages'
    if not any(x.get('id') == expected for x in lock['artifacts']):
        errors.append(f'missing native package lock {expected}')
if errors:
    print('\n'.join(errors), file=sys.stderr); raise SystemExit(1)
print('dossier validation passed')
