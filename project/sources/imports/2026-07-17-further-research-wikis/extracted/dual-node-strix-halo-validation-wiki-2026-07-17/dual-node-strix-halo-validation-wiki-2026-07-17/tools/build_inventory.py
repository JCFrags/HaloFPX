#!/usr/bin/env python3
"""Generate FILE-INVENTORY.csv and MANIFEST.sha256 for the unpacked bundle."""
from __future__ import annotations
import csv,hashlib,sys
from pathlib import Path

def digest(p:Path):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for c in iter(lambda:f.read(1024*1024),b''): h.update(c)
    return h.hexdigest()
def category(rel:Path): return rel.parts[0] if len(rel.parts)>1 else 'root'
def main():
    root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
    skip={'MANIFEST.sha256','FILE-INVENTORY.csv'}
    files=[p for p in root.rglob('*') if p.is_file() and p.name not in skip and not p.name.endswith(('.zip','.tar.gz')) and '__pycache__' not in p.parts]
    files.sort(key=lambda p:p.relative_to(root).as_posix())
    inv=root/'FILE-INVENTORY.csv'
    with inv.open('w',newline='',encoding='utf-8') as fh:
        w=csv.writer(fh); w.writerow(['path','category','bytes','sha256'])
        for p in files:
            rel=p.relative_to(root); w.writerow([rel.as_posix(),category(rel),p.stat().st_size,digest(p)])
    manifest_files=files+[inv]; manifest_files.sort(key=lambda p:p.relative_to(root).as_posix())
    (root/'MANIFEST.sha256').write_text(''.join(f"{digest(p)}  {p.relative_to(root).as_posix()}\n" for p in manifest_files),encoding='utf-8')
    print(f'inventory_files={len(files)}')
if __name__=='__main__': main()
