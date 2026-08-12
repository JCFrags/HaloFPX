#!/usr/bin/env python3
"""Resolve exact Hugging Face tree manifests without downloading model weights."""
from __future__ import annotations
import argparse, json, time, urllib.parse, urllib.request
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CANDS=json.loads((ROOT/'data/candidates.json').read_text())['candidates']
def fetch(url):
    req=urllib.request.Request(url,headers={'User-Agent':'llm-wiki-manifest/1.0'})
    with urllib.request.urlopen(req,timeout=120) as r:return json.load(r)
def resolve(c,outdir):
    p=c['provenance']; repo=p['quant_repo']; rev=p['revision']
    if rev.startswith('main-observed-'):rev='main'
    path=p['path'].strip('/')
    api='https://huggingface.co/api/models/'+repo+'/tree/'+urllib.parse.quote(rev,safe='')
    if path: api+='/'+urllib.parse.quote(path,safe='/')
    api+='?recursive=true&expand=true'
    items=fetch(api)
    files=[]
    for x in items:
        if x.get('type')!='file':continue
        lfs=x.get('lfs') or {}
        files.append({'path':x.get('path'),'bytes':x.get('size'),'git_oid':x.get('oid'),'lfs_sha256':lfs.get('oid'),'lfs_bytes':lfs.get('size'),'pointer_bytes':lfs.get('pointerSize')})
    manifest={'schema':'hf-tree-expanded-v1','candidate_id':c['id'],'repo':repo,'revision_requested':rev,'path':path,'api_url':api,'files':files,'total_bytes':sum((f.get('lfs_bytes') or f.get('bytes') or 0) for f in files),'generated_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())}
    outdir.mkdir(parents=True,exist_ok=True);(outdir/f"{c['id']}.json").write_text(json.dumps(manifest,indent=2))
    print(c['id'],len(files),manifest['total_bytes'])
def main():
    ap=argparse.ArgumentParser();ap.add_argument('candidate',nargs='?');ap.add_argument('--all',action='store_true');ap.add_argument('--outdir',default=str(ROOT/'manifests/refreshed'));a=ap.parse_args()
    chosen=CANDS if a.all else [c for c in CANDS if c['id']==a.candidate]
    if not chosen:ap.error('choose a candidate id or --all')
    failures=0
    for c in chosen:
        try:resolve(c,Path(a.outdir))
        except Exception as e:
            failures+=1
            print(c['id'],'ERROR',e)
    if failures:
        raise SystemExit(1)
if __name__=='__main__':main()
