#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"
mkdir -p results

python3 generate_fixtures.py --output fixtures --schema-sample ../schemas/sample-manifest.json > results/fixture-values.pretty.json

VALUES="fixtures/fixture-values.json"
COMPAT_U64="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["cachyllama"]["compat_u64_hex"])')"
CKPT="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["cachyllama"]["checkpoint"])')"
INDEX="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["cachyllama"]["index"])')"
SYSTEM="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["cachyllama"]["system"])')"
MANIFEST="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["manifest"])')"
OBJECT_ROOT="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["object_root"])')"
MANIFEST_HMAC_KEY="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["manifest_hmac_key"])')"
NAMESPACE_ID="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["namespace_id"])')"
COMPAT_SHA256="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["compatibility_fingerprint_sha256"])')"
CACHE_KEY_SHA256="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["cache_key_sha256"])')"
PROMPT_ROOT_SHA256="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["prompt_root_sha256"])')"
ENGINE_FAMILY="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["engine_family"])')"

python3 validate_cache.py --pretty cachyllama-checkpoint "fixtures/$CKPT" --expected-compat "$COMPAT_U64" > results/legacy-checkpoint.json
python3 validate_cache.py --pretty cachyllama-index "fixtures/$INDEX" --expected-compat "$COMPAT_U64" > results/legacy-index.json
python3 validate_cache.py --pretty cachyllama-system "fixtures/$SYSTEM" --expected-compat "$COMPAT_U64" > results/legacy-system.json
python3 validate_cache.py --pretty halofpx-manifest "fixtures/$MANIFEST" \
  --object-root "fixtures/$OBJECT_ROOT" \
  --manifest-hmac-key-file "fixtures/$MANIFEST_HMAC_KEY" > results/halofpx-catalog-entry.json
python3 validate_cache.py --pretty halofpx-manifest "fixtures/$MANIFEST" \
  --object-root "fixtures/$OBJECT_ROOT" \
  --manifest-hmac-key-file "fixtures/$MANIFEST_HMAC_KEY" \
  --expected-namespace "$NAMESPACE_ID" \
  --expected-compat "$COMPAT_SHA256" \
  --expected-cache-key "$CACHE_KEY_SHA256" \
  --expected-prompt-root "$PROMPT_ROOT_SHA256" \
  --expected-engine-family "$ENGINE_FAMILY" > results/halofpx-manifest.json

python3 -m unittest discover -s tests -v 2>&1 | tee results/unit-tests.txt
python3 fault_inject.py --fixtures fixtures --results results > results/fault-injection.stdout.json
python3 endurance_model.py --scenario-file ../tables/endurance-scenarios.csv --output results/endurance-report.md --json-output results/endurance-report.json > /dev/null

python3 - <<'PY_SUMMARY'
import json
import re
from pathlib import Path
r=Path('results')
files=['legacy-checkpoint.json','legacy-index.json','legacy-system.json','halofpx-catalog-entry.json','halofpx-manifest.json','fault-injection.json','endurance-report.json']
summary={
    'artifact': 'HaloFPX persistent KV-cache validation',
    'research_cut': '2026-07-17',
    'all_passed': True,
    'public_hit_emitted': False,
    'artifacts': {},
}
for name in files:
    summary['artifacts'][name]=json.loads((r/name).read_text())
unit_text=(r/'unit-tests.txt').read_text()
match=re.search(r'Ran (\d+) tests?', unit_text)
summary['unit_tests']={
    'passed': int(match.group(1)) if match else None,
    'output': 'unit-tests.txt',
}
fault=summary['artifacts']['fault-injection.json']
summary['fault_injection']={
    'passed': fault['all_passed'],
    'case_count': fault['case_count'],
    'report_json': 'fault-injection.json',
    'report_markdown': 'fault-injection.md',
}
if not fault['all_passed'] or summary['unit_tests']['passed'] is None:
    summary['all_passed']=False
catalog=summary['artifacts']['halofpx-catalog-entry.json']
if catalog['status']!='CATALOG_ENTRY_VALID' or catalog['eligible_for_hit'] or catalog['eligible_for_engine_import']:
    summary['all_passed']=False
manifest=summary['artifacts']['halofpx-manifest.json']
if manifest['status']!='IMPORT_CANDIDATE_VALID' or manifest['eligible_for_hit'] or not manifest['eligible_for_engine_import']:
    summary['all_passed']=False
for name in ('legacy-checkpoint.json','legacy-index.json','legacy-system.json'):
    result=summary['artifacts'][name]
    if result['eligible_for_hit'] or result['eligible_for_engine_import']:
        summary['all_passed']=False
for value in summary['artifacts'].values():
    if isinstance(value, dict) and value.get('eligible_for_hit'):
        summary['public_hit_emitted']=True
        summary['all_passed']=False
(r/'validation-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
if not summary['all_passed']:
    raise SystemExit(1)
print('Validation suite passed. See validation/results/.')
PY_SUMMARY
