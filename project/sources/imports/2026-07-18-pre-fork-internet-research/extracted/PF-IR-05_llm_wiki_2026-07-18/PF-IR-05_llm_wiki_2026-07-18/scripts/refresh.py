#!/usr/bin/env python3
"""Fail-closed refresh of immutable Git/Hugging Face revisions.

No branch/tag fallback is implemented. Large LFS objects are not materialized unless
--materialize-large-files is supplied explicitly.
"""
from __future__ import annotations
import argparse, json, os, re, shutil, subprocess, sys, time
from pathlib import Path

HEX40 = re.compile(r"^[0-9a-f]{40}$")
POINTER = re.compile(r"^version https://git-lfs.github.com/spec/v1\noid sha256:([0-9a-f]{64})\nsize ([0-9]+)\n?$", re.M)

def run(cmd, cwd=None, env=None):
    p=subprocess.run(cmd,cwd=cwd,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if p.returncode:
        raise RuntimeError(f"command failed ({p.returncode}): {' '.join(cmd)}\nSTDOUT:\n{p.stdout}\nSTDERR:\n{p.stderr}")
    return p.stdout.strip()

def clone_exact(url, revision, dest):
    if not HEX40.fullmatch(revision):
        raise ValueError(f"Refusing non-immutable revision: {revision}")
    if dest.exists(): shutil.rmtree(dest)
    dest.mkdir(parents=True)
    env=os.environ.copy(); env['GIT_LFS_SKIP_SMUDGE']='1'
    run(['git','init','-q'],cwd=dest,env=env)
    run(['git','remote','add','origin',url],cwd=dest,env=env)
    run(['git','fetch','--depth=1','origin',revision],cwd=dest,env=env)
    run(['git','checkout','--detach','FETCH_HEAD'],cwd=dest,env=env)
    actual=run(['git','rev-parse','HEAD'],cwd=dest,env=env)
    if actual != revision: raise RuntimeError(f"revision mismatch: expected {revision}, got {actual}")
    return actual

def parse_pointer(path):
    try: data=path.read_text(encoding='utf-8')
    except (UnicodeDecodeError,OSError): return None
    m=POINTER.fullmatch(data)
    return {'sha256':m.group(1),'bytes':int(m.group(2))} if m else None

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--package-root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--workdir',type=Path,default=Path('pf-ir-05-refresh-cache'))
    ap.add_argument('--materialize-large-files',action='store_true')
    ap.add_argument('--candidate')
    args=ap.parse_args()
    root=args.package_root.resolve(); args.workdir=args.workdir.resolve(); args.workdir.mkdir(parents=True,exist_ok=True)
    data=json.loads((root/'manifests/candidates.json').read_text())['candidates']
    qopts={x['candidate_id']:x for x in json.loads((root/'manifests/quantization_options.json').read_text())['candidates']}
    report={'started_at':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'materialized_large_files':args.materialize_large_files,'candidates':[]}
    for c in data:
        if args.candidate and c['candidate_id']!=args.candidate: continue
        art=json.loads((root/'manifests/artifacts'/f"{c['artifact_key']}.json").read_text())
        rows=[]
        repos=[
          ('publisher',c['publisher_identity']['repo'],c['publisher_identity']['revision']),
          ('artifact',art['repository'],art['revision'])]
        for role,repo,rev in repos:
            dest=args.workdir/f"{c['candidate_id']}-{role}"
            actual=clone_exact(f"https://huggingface.co/{repo}",rev,dest)
            entry={'role':role,'repo':repo,'revision':actual,'path':str(dest)}
            if role=='artifact':
                pointer_rows=[]
                for shard in art['shards']:
                    p=dest/art['selected_path']/shard['name']
                    pointer=parse_pointer(p) if p.exists() else None
                    pointer_rows.append({'name':shard['name'],'exists':p.exists(),'pointer':pointer,'manifest_bytes':shard.get('bytes'),'manifest_sha256':shard.get('sha256')})
                entry['pointers']=pointer_rows
                qrow=qopts.get(c['candidate_id'])
                if qrow:
                    entry['quantization_options']=[{'quantization':o['quantization'],'path_exists':(dest/o['quantization']).exists(),'manifest_display_size':o['display_size']} for o in qrow['options']]
                if args.materialize_large_files:
                    if shutil.which('git-lfs') is None and run(['git','lfs','version'],cwd=dest) is None:
                        raise RuntimeError('git-lfs required')
                    run(['git','lfs','pull','--include',art['selected_path']+'/*','--exclude',''],cwd=dest)
            rows.append(entry)
        report['candidates'].append({'candidate_id':c['candidate_id'],'repositories':rows})
    out=root/'validation/refresh-report.json'; out.write_text(json.dumps(report,indent=2)+'\n')
    print(out)
if __name__=='__main__': main()
