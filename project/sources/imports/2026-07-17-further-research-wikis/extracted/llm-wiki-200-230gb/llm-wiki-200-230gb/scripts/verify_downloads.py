#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path
def sha256(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(16*1024*1024),b''):h.update(b)
    return h.hexdigest()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--download-root',required=True);ap.add_argument('--manifest');ap.add_argument('--manifest-dir');a=ap.parse_args()
    manifests=[]
    if a.manifest:manifests=[Path(a.manifest)]
    elif a.manifest_dir:manifests=sorted(Path(a.manifest_dir).glob('*.json'))
    else:ap.error('use --manifest or --manifest-dir')
    root=Path(a.download_root);failed=0
    for mp in manifests:
        m=json.loads(mp.read_text())
        files=m.get('files') or m.get('huggingface',{}).get('known_lfs',[])
        for x in files:
            rel=x.get('path') or x.get('file'); expected=x.get('lfs_sha256') or x.get('sha256')
            if not expected:continue
            p=root/rel
            if not p.exists():print('MISSING',p);failed+=1;continue
            got=sha256(p);ok=got==expected;print('OK' if ok else 'BAD',p,got)
            failed+=0 if ok else 1
    raise SystemExit(1 if failed else 0)
if __name__=='__main__':main()
