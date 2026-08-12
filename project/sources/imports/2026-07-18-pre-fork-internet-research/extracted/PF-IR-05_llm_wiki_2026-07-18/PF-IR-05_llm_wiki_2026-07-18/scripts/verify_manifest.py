#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, re, sys
from pathlib import Path

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--package-root',type=Path,default=Path(__file__).resolve().parents[1]); ap.add_argument('--verify-package-checksums',action='store_true'); args=ap.parse_args()
    root=args.package_root.resolve(); errors=[]; notes=[]
    for p in sorted((root/'manifests/artifacts').glob('*.json')):
        a=json.loads(p.read_text()); shards=a['shards']; exact=[s for s in shards if s.get('bytes') is not None and s.get('sha256')]
        for s in exact:
            if not re.fullmatch(r'[0-9a-f]{64}',s['sha256']): errors.append(f"{p.name}: bad SHA {s['name']}")
        if a.get('exact_total_bytes') is not None:
            if len(exact)!=len(shards): errors.append(f"{p.name}: exact total but incomplete shards")
            total=sum(s['bytes'] for s in shards)
            if total!=a['exact_total_bytes']: errors.append(f"{p.name}: total mismatch {total} != {a['exact_total_bytes']}")
        else: notes.append(f"{p.name}: exact total unavailable ({a['manifest_completeness']})")
    if args.verify_package_checksums:
        sums=root/'checksums/SHA256SUMS'
        for line in sums.read_text().splitlines():
            if not line.strip(): continue
            expected,rel=line.split('  ',1); target=root/rel
            if not target.exists(): errors.append(f"missing {rel}")
            elif sha(target)!=expected: errors.append(f"checksum mismatch {rel}")
    print(json.dumps({'ok':not errors,'errors':errors,'notes':notes},indent=2))
    return 1 if errors else 0
if __name__=='__main__': raise SystemExit(main())
