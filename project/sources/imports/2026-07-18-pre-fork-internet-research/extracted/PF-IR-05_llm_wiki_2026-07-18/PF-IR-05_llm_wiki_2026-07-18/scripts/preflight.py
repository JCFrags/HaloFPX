#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, subprocess, sys, time
from pathlib import Path

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(8*1024*1024),b''): h.update(b)
    return h.hexdigest()

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('candidate'); ap.add_argument('--package-root',type=Path,default=Path(__file__).resolve().parents[1]); ap.add_argument('--files-root',type=Path); ap.add_argument('--verify-files',action='store_true'); args=ap.parse_args()
    root=args.package_root.resolve(); candidates=json.loads((root/'manifests/candidates.json').read_text())['candidates']; c=next((x for x in candidates if x['candidate_id']==args.candidate),None)
    if not c: raise SystemExit('unknown candidate')
    a=json.loads((root/'manifests/artifacts'/f"{c['artifact_key']}.json").read_text())
    report={'candidate_id':args.candidate,'artifact_revision':a['revision'],'started_at':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'files':[],'execution':'NOT_RUN'}
    if args.verify_files:
        if not args.files_root: raise SystemExit('--files-root required with --verify-files')
        for s in a['shards']:
            p=args.files_root/s['name']; row={'name':s['name'],'exists':p.exists()}
            if p.exists():
                row['bytes']=p.stat().st_size; row['sha256']=sha(p)
                row['bytes_match']=s.get('bytes') is not None and row['bytes']==s['bytes']
                row['sha256_match']=s.get('sha256') is not None and row['sha256']==s['sha256']
            report['files'].append(row)
    out=root/'validation'/f"{args.candidate}-inventory.json"; out.write_text(json.dumps(report,indent=2)+'\n'); print(out)
if __name__=='__main__': main()
