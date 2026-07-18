#!/usr/bin/env python3
"""Create deterministic PF-IR-10 inventories and archives.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import argparse, gzip, hashlib, json, os, stat, tarfile, zipfile
from pathlib import Path
from typing import Iterable

ZIP_TIME=(1980,1,1,0,0,0)
TAR_MTIME=0


def digest(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest()


def files(root: Path, *, exclude: set[str] | None=None) -> list[Path]:
    exclude=exclude or set(); out=[]
    for p in sorted(root.rglob('*')):
        if p.is_symlink(): raise RuntimeError(f'symlink prohibited: {p}')
        if p.is_file() and p.relative_to(root).as_posix() not in exclude: out.append(p)
    return out


def license_for(rp: str) -> str:
    if rp.startswith(('evidence/raw/source-ranges/llama.cpp/',
                      'evidence/raw/source-ranges/ROCmFPX/',
                      'evidence/raw/source-ranges/CachyLLama/')):
        return 'MIT'
    if rp in {'evidence/licenses/llama.cpp-LICENSE.txt','evidence/licenses/ROCmFPX-LICENSE.txt','evidence/licenses/CachyLLama-LICENSE.txt'}: return 'MIT'
    if rp=='evidence/licenses/ROCmFPX-THIRD_PARTY_NOTICES.md': return 'notice-record'
    if rp in {'LICENSE','evidence/licenses/CC0-1.0.txt'}: return 'CC0-1.0-legal-code'
    return 'CC0-1.0'


def write_inventory(root: Path) -> None:
    target=root/'manifests/file-inventory.jsonl'
    rows=[]
    for p in files(root,exclude={'MANIFEST.sha256','manifests/file-inventory.jsonl'}):
        rp=p.relative_to(root).as_posix(); mode=stat.S_IMODE(p.stat().st_mode)
        rows.append({'path':rp,'sha256':digest(p),'length':p.stat().st_size,'mode':format(mode,'04o'),'license':license_for(rp)})
    target.write_text(''.join(json.dumps(r,sort_keys=True,separators=(',',':'))+'\n' for r in rows),encoding='utf-8',newline='\n')


def write_manifest(root: Path) -> None:
    lines=[]
    for p in files(root,exclude={'MANIFEST.sha256'}):
        rp=p.relative_to(root).as_posix(); lines.append(f'{digest(p)}  {rp}\n')
    (root/'MANIFEST.sha256').write_text(''.join(lines),encoding='utf-8',newline='\n')


def verify_manifest(root: Path) -> None:
    for line in (root/'MANIFEST.sha256').read_text(encoding='utf-8').splitlines():
        expected,rp=line.split('  ',1); p=root/rp
        if not p.is_file() or digest(p)!=expected: raise RuntimeError(f'manifest mismatch: {rp}')


def add_zip(root: Path, out: Path) -> None:
    prefix=root.name+'/'
    with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in files(root):
            rp=p.relative_to(root).as_posix(); zi=zipfile.ZipInfo(prefix+rp,ZIP_TIME)
            zi.create_system=3; mode=stat.S_IMODE(p.stat().st_mode); zi.external_attr=((stat.S_IFREG|mode)<<16)
            zi.compress_type=zipfile.ZIP_DEFLATED; z.writestr(zi,p.read_bytes(),compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)


def add_tar(root: Path, out: Path) -> None:
    with out.open('wb') as raw, gzip.GzipFile(filename='',mode='wb',fileobj=raw,mtime=0,compresslevel=9) as gz, tarfile.open(mode='w',fileobj=gz,format=tarfile.PAX_FORMAT) as t:
        dirs=[root]+[p for p in sorted(root.rglob('*')) if p.is_dir()]
        for d in dirs:
            ti=tarfile.TarInfo(root.name if d==root else f'{root.name}/{d.relative_to(root).as_posix()}')
            ti.type=tarfile.DIRTYPE; ti.mode=0o755; ti.mtime=TAR_MTIME; ti.uid=0; ti.gid=0; ti.uname=''; ti.gname=''; t.addfile(ti)
        for p in files(root):
            rp=f'{root.name}/{p.relative_to(root).as_posix()}'; ti=tarfile.TarInfo(rp); data=p.read_bytes()
            ti.size=len(data); ti.mode=stat.S_IMODE(p.stat().st_mode); ti.mtime=TAR_MTIME; ti.uid=0; ti.gid=0; ti.uname=''; ti.gname=''
            import io; t.addfile(ti,io.BytesIO(data))


def main() -> int:
    ap=argparse.ArgumentParser(); ap.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]); ap.add_argument('--out-dir',type=Path,default=Path('/mnt/data'))
    ns=ap.parse_args(); root=ns.root.resolve(); out=ns.out_dir.resolve(); out.mkdir(parents=True,exist_ok=True)
    (root/'qualification/PACKAGE-STATUS.json').write_text(json.dumps({'schema_version':1,'source_date_epoch':0,'zip_timestamp':'1980-01-01T00:00:00','tar_mtime':0,'symlinks_allowed':False,'manifest_excludes_itself':True,'claim_labels':['SELF-GENERATED']},sort_keys=True,indent=2)+'\n',encoding='utf-8')
    write_inventory(root); write_manifest(root); verify_manifest(root)
    zip_path=out/(root.name+'.zip'); tar_path=out/(root.name+'.tar.gz')
    add_zip(root,zip_path); add_tar(root,tar_path)
    for p in (zip_path,tar_path): (Path(str(p)+'.sha256')).write_text(f'{digest(p)}  {p.name}\n',encoding='utf-8')
    report={'root':str(root),'file_count':len(files(root)),'manifest_sha256':digest(root/'MANIFEST.sha256'),'archives':[{'path':str(p),'sha256':digest(p),'length':p.stat().st_size} for p in (zip_path,tar_path)]}
    report_path=out/(root.name+'-package-report.json'); report_path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,sort_keys=True))
    return 0
if __name__=='__main__': raise SystemExit(main())
