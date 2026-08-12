#!/usr/bin/env python3
"""Audit run-manifest completeness and raw-file hashes against provenance policy."""
from __future__ import annotations
import argparse,hashlib,json,re,sys
from pathlib import Path
import yaml
from jsonschema import Draft202012Validator

def sha(p:Path):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for c in iter(lambda:f.read(1024*1024),b''): h.update(c)
    return h.hexdigest()
def expand(values,part):
    out=[]
    m=re.fullmatch(r'([^\[]+)\[\*\]',part)
    for value in values:
        if not isinstance(value,dict): continue
        if m:
            v=value.get(m.group(1)); out.extend(v if isinstance(v,list) else [])
        elif part in value: out.append(value[part])
    return out
def get_values(obj,path):
    vals=[obj]
    for part in path.split('.'): vals=expand(vals,part)
    return vals
def present(values):
    return bool(values) and all(v is not None and v!='' and v!=[] and v!={} for v in values)
def scan_keys(value,patterns,prefix=''):
    hits=[]
    if isinstance(value,dict):
        for k,v in value.items():
            keypath=f'{prefix}.{k}' if prefix else k
            if any(p.search(k) for p in patterns) and v not in (None,'','REDACTED','redacted'): hits.append(keypath)
            hits.extend(scan_keys(v,patterns,keypath))
    elif isinstance(value,list):
        for i,v in enumerate(value): hits.extend(scan_keys(v,patterns,f'{prefix}[{i}]'))
    return hits
def main():
    root=Path(__file__).resolve().parents[1]
    p=argparse.ArgumentParser(description=__doc__); p.add_argument('manifest',type=Path)
    p.add_argument('--policy',type=Path,default=root/'config/provenance-requirements.yaml')
    p.add_argument('--schema',type=Path,default=root/'schemas/run-manifest.schema.json'); p.add_argument('--run-dir',type=Path); p.add_argument('--output',type=Path)
    a=p.parse_args(); manifest=json.loads(a.manifest.read_text()); policy=yaml.safe_load(a.policy.read_text()); schema=json.loads(a.schema.read_text())
    schema_errors=[f'{e.json_path}: {e.message}' for e in Draft202012Validator(schema).iter_errors(manifest)]
    required=list(policy['required_manifest_paths']['common'])
    if manifest.get('topology')=='dual': required+=policy['required_manifest_paths'].get('dual',[])
    required+=policy['required_manifest_paths'].get('cache_state',{}).get(manifest.get('cache_state'),[])
    missing=[path for path in required if not present(get_values(manifest,path))]
    run_dir=(a.run_dir or a.manifest.parent).resolve(); hash_fail=[]; record_types=[]
    for f in manifest.get('raw_files',[]):
        record_types.append(f.get('record_type')); path=run_dir/f['path']
        if not path.exists(): hash_fail.append({'path':f['path'],'error':'missing'})
        elif path.stat().st_size!=f['bytes'] or sha(path)!=f['sha256']: hash_fail.append({'path':f['path'],'error':'size_or_sha256_mismatch'})
    needed_types=list(policy['required_raw_record_types']['default'])
    if manifest.get('topology')=='dual': needed_types+=policy['required_raw_record_types'].get('dual',[])
    if manifest.get('experiment_id') in {'EXP-007','EXP-008','EXP-009'}: needed_types+=policy['required_raw_record_types'].get('streaming',[])
    if manifest.get('experiment_id') in {'EXP-016','EXP-017','EXP-018'}: needed_types+=policy['required_raw_record_types'].get('faults',[])
    missing_types=sorted(set(needed_types)-set(record_types))
    patterns=[re.compile(x) for x in policy.get('forbidden_manifest_key_patterns',[])]
    secret_hits=scan_keys(manifest,patterns)
    completeness=(len(required)-len(missing))/len(required) if required else 1.0
    result={'schema_version':1,'audit_type':'provenance','manifest':str(a.manifest),'schema_valid':not schema_errors,'schema_errors':schema_errors,
            'required_fields':len(required),'missing_fields':missing,'completeness':completeness,'raw_hashes_verified':not hash_fail,
            'raw_hash_failures':hash_fail,'missing_required_record_types':missing_types,'forbidden_secret_key_hits':secret_hits,
            'status':'PASS' if not schema_errors and not missing and not hash_fail and not missing_types and not secret_hits else 'INSUFFICIENT_EVIDENCE',
            'note':'A passing provenance audit confirms evidence identity/integrity, not machine performance or correctness.'}
    text=json.dumps(result,indent=2); print(text); a.output and a.output.write_text(text+'\n'); return 0 if result['status']=='PASS' else 1
if __name__=='__main__': sys.exit(main())
